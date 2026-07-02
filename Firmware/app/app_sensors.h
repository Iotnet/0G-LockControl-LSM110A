/**
  ******************************************************************************
  * @file    app_sensors.h
  * @brief   Capa de aplicacion de sensores del 0G LockControl.
  *          Une los drivers (LIS2DW12 + reed) con el sequencer del SDK y el
  *          stack Sigfox: evento EXTI -> task -> payload -> TX.
  * @author  Yahir Flores - 0G IoT Solutions
  *
  * @note    Flujo de un evento:
  *          1. EXTI (PA0/PA1) -> HAL_GPIO_EXTI_Callback -> app_sensors_notify_*_isr()
  *             (contexto ISR: solo flags + UTIL_SEQ_SetTask, sin I/O)
  *          2. El sequencer ejecuta la task correspondiente en main loop:
  *             lee el sensor, arma el payload de 12 bytes y dispara TX Sigfox
  *             con cooldown de 60 s entre transmisiones.
  *
  * @note    Tareas del backlog M2 cubiertas: Y1 (config wake-up en boot),
  *          Y2 (procesamiento en main loop), Y3 (enganche a stm32_seq.h).
  *          Expone app_sensors_get_last_event() para F3/F4 (Franco).
  ******************************************************************************
  */

#ifndef APP_SENSORS_H
#define APP_SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include "payload_codec.h"

/* Umbral de wake-up del acelerometro en mg (spec: golpe/manipulacion) */
#define APP_SENSORS_WAKEUP_THRESHOLD_MG   200U

/* Cooldown minimo entre transmisiones Sigfox (F3: 60 s) */
#define APP_SENSORS_TX_COOLDOWN_MS        60000U

/* Repeticiones adicionales del frame Sigfox (2 -> 3 frames total, estandar) */
#define APP_SENSORS_TX_REPEATS            2U

/**
 * @brief Ultimo evento de sensor procesado (para F3/F4).
 */
typedef struct {
    uint8_t  fuente;        /* PAYLOAD_FUENTE_ACCEL o PAYLOAD_FUENTE_MAGNETICO */
    uint8_t  eje;           /* PAYLOAD_EJE_* (solo aplica a accel)             */
    uint16_t magnitud_mg;   /* umbral superado (accel) o 0 (reed)             */
    uint8_t  reed_state;    /* PAYLOAD_MAG_* al momento del evento            */
    uint32_t tick_ms;       /* HAL_GetTick() al procesar el evento            */
} app_sensor_event_t;

/**
 * @brief Inicializa sensores y registra las tasks en el sequencer.
 *
 *        - Verifica WHO_AM_I del LIS2DW12 (log: "Accel OK, WHO_AM_I=0x44")
 *        - Configura wake-up a APP_SENSORS_WAKEUP_THRESHOLD_MG con INT1
 *        - Inicializa el reed switch y loguea su estado inicial
 *        - Registra CFG_SEQ_Task_ACCEL_EVENT / CFG_SEQ_Task_REED_EVENT
 *
 * @note  Llamar desde MX_Sigfox_Init() DESPUES de SystemApp_Init(): necesita
 *        el trace UART y el sequencer ya inicializados. El BSP (I2C1 + EXTI)
 *        debe estar inicializado antes (BSP_LockControl_Init en main).
 */
void app_sensors_init(void);

/**
 * @brief Re-agenda tasks si quedo algun flag pendiente sin procesar.
 *        Red de seguridad para eventos que llegan en ventanas raras (p.ej.
 *        entre BSP init y registro de tasks). Llamar en cada iteracion de
 *        MX_Sigfox_Process(). Costo: dos comparaciones.
 */
void app_sensors_poll(void);

/**
 * @brief Notificacion desde ISR de EXTI0 (PA0, INT1 del LIS2DW12).
 *        Solo marca flag y agenda task. Seguro para contexto ISR.
 */
void app_sensors_notify_accel_isr(void);

/**
 * @brief Notificacion desde ISR de EXTI1 (PA1, reed switch).
 *        Aplica debounce del driver; solo agenda task si el evento es valido.
 */
void app_sensors_notify_reed_isr(void);

/**
 * @brief Copia el ultimo evento procesado (API para F3/F4).
 * @retval true si ha ocurrido al menos un evento desde el boot.
 */
bool app_sensors_get_last_event(app_sensor_event_t *out);

/**
 * @brief Conteo acumulado de eventos desde el boot (campo del payload).
 */
uint16_t app_sensors_get_event_count(void);

/**
 * @brief Estado actual del reed (lectura directa, sin debounce).
 *        Para el heartbeat de F4. 0 = cerrado, 1 = abierto (PAYLOAD_MAG_*).
 */
uint8_t app_sensors_read_reed_now(void);

/**
 * @brief Bateria estimada en % (curva lineal CR2450: 3.0 V = 100, 2.0 V = 0).
 */
uint8_t app_sensors_battery_pct(void);

/**
 * @brief Temperatura del MCU en grados C (sensor interno, para el payload).
 */
int8_t app_sensors_temp_c(void);

#endif /* APP_SENSORS_H */
