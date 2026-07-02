/**
  ******************************************************************************
  * @file    app_sensors.c
  * @brief   Implementacion de la capa de sensores: eventos -> payload -> TX.
  * @author  Yahir Flores - 0G IoT Solutions
  ******************************************************************************
  */

#include <stdio.h>

#include "app_sensors.h"
#include "bsp_lockcontrol.h"
#include "lis2dw12.h"
#include "reed_switch.h"

#include "sys_app.h"          /* APP_LOG */
#include "adc_if.h"           /* SYS_GetBatteryLevel, SYS_GetTemperatureLevel */
#include "stm32_seq.h"
#include "utilities_def.h"    /* CFG_SEQ_Task_ACCEL_EVENT / CFG_SEQ_Task_REED_EVENT */
#include "st_sigfox_api.h"    /* SIGFOX_API_send_frame */

/* ----------------------------- Estado ----------------------------- */

static lis2dw12_t    s_accel;
static reed_switch_t s_reed;

/* Guard contra IRQs que lleguen entre BSP init y registro de tasks:
   agendar una task no registrada en el sequencer seria comportamiento
   indefinido, asi que las notify_*_isr() ignoran todo hasta init completo. */
static volatile bool s_initialized = false;

static app_sensor_event_t s_last_event;
static bool     s_has_event    = false;
static uint16_t s_event_count  = 0;

static uint32_t s_last_tx_tick = 0;
static bool     s_tx_done_once = false;   /* el primer TX nunca se suprime */

/* ----------------------- Prototipos privados ----------------------- */

static void AccelEventTask(void);
static void ReedEventTask(void);
static void TrySendAlarm(uint8_t fuente);

/* --------------------------- API publica --------------------------- */

void app_sensors_init(void)
{
    uint8_t who = 0;

    /* --- Acelerometro (Y1) --- */
    lis2dw12_init_handle(&s_accel, &hi2c1);

    /* El LIS2DW12 tarda ~20 ms en salir de power-on; reintenta el WHO_AM_I
       para no depender del orden exacto del boot. */
    lis2dw12_status_t st = LIS2DW12_ERR_I2C;
    for (int i = 0; i < 3 && st != LIS2DW12_OK; i++) {
        st = lis2dw12_who_am_i(&s_accel, &who);
        if (st != LIS2DW12_OK) {
            HAL_Delay(20);
        }
    }

    if (st == LIS2DW12_OK) {
        APP_LOG(TS_ON, VLEVEL_L, "Accel OK, WHO_AM_I=0x%02X\r\n", who);

        if (lis2dw12_config_wakeup(&s_accel, APP_SENSORS_WAKEUP_THRESHOLD_MG) == LIS2DW12_OK) {
            APP_LOG(TS_ON, VLEVEL_L, "Accel wake-up cfg: %u mg -> INT1 (PA0)\r\n",
                    (unsigned int)APP_SENSORS_WAKEUP_THRESHOLD_MG);
        } else {
            APP_LOG(TS_ON, VLEVEL_L, "ERROR: accel config_wakeup fallo\r\n");
        }
    } else {
        /* Sin acelerometro el producto sigue operando con el reed:
           degradacion parcial, no bloquear el boot. */
        APP_LOG(TS_ON, VLEVEL_L, "ERROR: accel WHO_AM_I=0x%02X (esperado 0x44), status=%d\r\n",
                who, (int)st);
    }

    /* --- Reed switch --- */
    reed_switch_init(&s_reed, REED_GPIO_PORT, REED_PIN, REED_DEBOUNCE_MS_DEFAULT);
    APP_LOG(TS_ON, VLEVEL_L, "Reed inicial: %s\r\n",
            (reed_switch_read_now(&s_reed) == REED_OPEN) ? "ABIERTO" : "CERRADO");

    /* --- Tasks del sequencer (Y3) --- */
    UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_ACCEL_EVENT), UTIL_SEQ_RFU, AccelEventTask);
    UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_REED_EVENT),  UTIL_SEQ_RFU, ReedEventTask);

    s_initialized = true;

    /* Recupera eventos que hayan llegado durante el boot (flag ya seteado
       por el driver pero task aun no agendada). */
    app_sensors_poll();
}

void app_sensors_poll(void)
{
    if (!s_initialized) {
        return;
    }
    if (lis2dw12_has_pending_event(&s_accel)) {
        UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_ACCEL_EVENT), CFG_SEQ_Prio_0);
    }
    if (reed_switch_has_pending_event(&s_reed)) {
        UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_REED_EVENT), CFG_SEQ_Prio_0);
    }
}

void app_sensors_notify_accel_isr(void)
{
    if (!s_initialized) {
        return;
    }
    lis2dw12_on_interrupt(&s_accel);
    UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_ACCEL_EVENT), CFG_SEQ_Prio_0);
}

void app_sensors_notify_reed_isr(void)
{
    if (!s_initialized) {
        return;
    }
    reed_switch_on_interrupt(&s_reed);
    /* on_interrupt aplica debounce: solo agenda si el evento sobrevivio */
    if (reed_switch_has_pending_event(&s_reed)) {
        UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_REED_EVENT), CFG_SEQ_Prio_0);
    }
}

bool app_sensors_get_last_event(app_sensor_event_t *out)
{
    /* s_last_event solo se escribe en contexto de task (sequencer), nunca
       en ISR, asi que leerlo desde otra task no requiere critical section. */
    if (out == NULL || !s_has_event) {
        return false;
    }
    *out = s_last_event;
    return true;
}

uint16_t app_sensors_get_event_count(void)
{
    return s_event_count;
}

uint8_t app_sensors_read_reed_now(void)
{
    return (reed_switch_read_now(&s_reed) == REED_OPEN)
           ? PAYLOAD_MAG_ABIERTO : PAYLOAD_MAG_CERRADO;
}

uint8_t app_sensors_battery_pct(void)
{
    uint16_t mv = SYS_GetBatteryLevel();   /* mV */

    /* CR2450: 3.0 V fresca, 2.0 V fin de vida. La curva real es plana y cae
       al final; la aproximacion lineal basta para el MVP. */
    if (mv >= 3000U) {
        return 100U;
    }
    if (mv <= 2000U) {
        return 0U;
    }
    return (uint8_t)((mv - 2000U) / 10U);
}

int8_t app_sensors_temp_c(void)
{
    /* SYS_GetTemperatureLevel devuelve Q8.8: >>8 = grados C enteros */
    int16_t t = (int16_t)(SYS_GetTemperatureLevel() >> 8);

    /* Rango representable del byte 7 del payload (offset +40) */
    if (t > 87)  { t = 87;  }
    if (t < -40) { t = -40; }
    return (int8_t)t;
}

/* ------------------------ Tasks del sequencer ------------------------ */

static void AccelEventTask(void)
{
    lis2dw12_event_t evt;

    lis2dw12_status_t st = lis2dw12_process_event(&s_accel, &evt);
    if (st != LIS2DW12_OK) {
        APP_LOG(TS_ON, VLEVEL_L, "ERROR: accel process_event status=%d\r\n", (int)st);
        return;
    }
    if (!evt.detected) {
        /* INT1 sin WU_IA: rebote del pin o lectura tardia, no es evento */
        return;
    }

    s_event_count++;

    s_last_event.fuente      = PAYLOAD_FUENTE_ACCEL;
    /* El payload solo lleva un eje: prioridad X > Y > Z si disparo multiple */
    s_last_event.eje         = evt.axis_x ? PAYLOAD_EJE_X :
                               evt.axis_y ? PAYLOAD_EJE_Y :
                               evt.axis_z ? PAYLOAD_EJE_Z : PAYLOAD_EJE_NINGUNO;
    /* El wake-up del LIS2DW12 no reporta magnitud del impacto; se informa el
       umbral superado. Mejora futura: leer OUT_X/Y/Z al procesar. */
    s_last_event.magnitud_mg = APP_SENSORS_WAKEUP_THRESHOLD_MG;
    s_last_event.reed_state  = app_sensors_read_reed_now();
    s_last_event.tick_ms     = HAL_GetTick();
    s_has_event = true;

    APP_LOG(TS_ON, VLEVEL_L, "EVT accel: src=0x%02X eje=%u conteo=%u\r\n",
            evt.raw_src, (unsigned int)s_last_event.eje, (unsigned int)s_event_count);

    TrySendAlarm(PAYLOAD_FUENTE_ACCEL);
}

static void ReedEventTask(void)
{
    reed_state_t rst;

    reed_switch_process_event(&s_reed, &rst);

    s_event_count++;

    s_last_event.fuente      = PAYLOAD_FUENTE_MAGNETICO;
    s_last_event.eje         = PAYLOAD_EJE_NINGUNO;
    s_last_event.magnitud_mg = 0;
    s_last_event.reed_state  = (rst == REED_OPEN) ? PAYLOAD_MAG_ABIERTO
                                                  : PAYLOAD_MAG_CERRADO;
    s_last_event.tick_ms     = HAL_GetTick();
    s_has_event = true;

    APP_LOG(TS_ON, VLEVEL_L, "EVT reed: %s conteo=%u\r\n",
            (rst == REED_OPEN) ? "ABIERTO" : "CERRADO",
            (unsigned int)s_event_count);

    TrySendAlarm(PAYLOAD_FUENTE_MAGNETICO);
}

/* --------------------------- TX Sigfox --------------------------- */

/**
 * @brief Arma el payload de alarma y lo transmite si el cooldown lo permite.
 *
 * @note  Corre en contexto de task: SIGFOX_API_send_frame es bloqueante
 *        (~7 s en RC2: 3 frames de 12 bytes a 600 bps). Los eventos que
 *        lleguen durante el TX quedan en flags y se procesan al terminar.
 *        Durante el cooldown solo se suprime el TX; el conteo de eventos
 *        sigue acumulando y viaja en la siguiente transmision.
 */
static void TrySendAlarm(uint8_t fuente)
{
    uint32_t now = HAL_GetTick();

    if (s_tx_done_once && (now - s_last_tx_tick) < APP_SENSORS_TX_COOLDOWN_MS) {
        APP_LOG(TS_ON, VLEVEL_L, "TX suprimido: cooldown %lu s restantes (conteo=%u)\r\n",
                (unsigned long)((APP_SENSORS_TX_COOLDOWN_MS - (now - s_last_tx_tick)) / 1000U),
                (unsigned int)s_event_count);
        return;
    }

    payload_t p;
    uint8_t   buf[PAYLOAD_LEN];
    uint8_t   dl[8] = {0};   /* requerido por la API aunque no haya downlink */

    p.tipo           = PAYLOAD_TIPO_ALARMA;
    p.fuente         = fuente;
    p.magnitud_mg    = s_last_event.magnitud_mg;
    p.eje            = s_last_event.eje;
    p.magnetico      = s_last_event.reed_state;
    p.bateria_pct    = app_sensors_battery_pct();
    p.temp_c         = app_sensors_temp_c();
    p.conteo_eventos = s_event_count;

    if (payload_encode(&p, buf) != PAYLOAD_OK) {
        APP_LOG(TS_ON, VLEVEL_L, "ERROR: payload_encode fallo\r\n");
        return;
    }

    /* Hex dump del frame para validar contra el backend (F5) */
    char hex[2 * PAYLOAD_LEN + 1];
    for (int i = 0; i < PAYLOAD_LEN; i++) {
        snprintf(&hex[2 * i], 3, "%02X", buf[i]);
    }
    APP_LOG(TS_ON, VLEVEL_L, "TX Sigfox: %s\r\n", hex);

    sfx_error_t err = SIGFOX_API_send_frame(buf, PAYLOAD_LEN, dl,
                                            APP_SENSORS_TX_REPEATS, SFX_FALSE);
    if (err == SFX_ERR_NONE) {
        s_last_tx_tick = now;
        s_tx_done_once = true;
        APP_LOG(TS_ON, VLEVEL_L, "TX OK\r\n");
    } else {
        APP_LOG(TS_ON, VLEVEL_L, "ERROR: TX Sigfox 0x%04X\r\n", (unsigned int)err);
    }
}
