/**
  ******************************************************************************
  * @file    hall_door_app.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Monitoreo de apertura de puerta: sensor Hall omnipolar en PA1
  *          (HALL_DOOR, EXTI1 ambos flancos, configurado en el .ioc).
  *          Envia uplink Sigfox al ABRIR y al CERRAR, con debounce por timer,
  *          ventana minima entre uplinks y tope diario.
  *
  *          Flujo:  EXTI (flanco) -> re-arma DebounceTimer (one-shot)
  *                  DebounceTimer -> agenda CFG_SEQ_Task_DoorTx
  *                  Task          -> lee nivel estable; si != ultimo TX y la
  *                                   ventana lo permite -> TX Sigfox
  *                  GapTimer      -> reabre ventana y re-chequea (el estado
  *                                   final de una rafaga nunca se pierde)
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hall_door_app.h"
#include "app_features.h"

#if USE_DOOR_APP   /* ==== Modulo completo condicionado por feature flag ==== */

#include <stdint.h>
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "stm32_systime.h"
#include "utilities_def.h"
#include "sys_app.h"
#include "sigfox_types.h"
#include "st_sigfox_api.h"

/* Valores del byte de estado en el payload */
#define DOOR_PL_OPEN    0x00U   /* puerta ABIERTA (sin iman)     */
#define DOOR_PL_CLOSED  0x02U   /* puerta CERRADA (iman presente)*/

/* === Estado === */
static UTIL_TIMER_Object_t DebounceTimer;   /* one-shot: confirma nivel estable */
static UTIL_TIMER_Object_t GapTimer;        /* one-shot: ventana entre uplinks  */
static UTIL_TIMER_Object_t DayTimer;        /* periodico 24 h: resetea el tope  */
static UTIL_TIMER_Object_t BootSyncTimer;   /* one-shot: pide hora tras el boot */

static volatile uint8_t gapLocked = 0;      /* 1 = dentro de la ventana minima  */
static uint8_t  lastTxState = 0xFF;         /* estado del ultimo TX (0xFF=nunca)*/
static uint16_t eventCount  = 0;            /* eventos confirmados (wrap 16 bit)*/
static uint32_t txToday     = 0;            /* uplinks en las ultimas 24 h      */
static uint32_t daysSinceSync = 0;          /* dias desde el ultimo sync OK     */

#define DOOR_MS_PER_DAY  (24UL * 60UL * 60UL * 1000UL)

/* === Prototipos === */
static void    DoorHall_Task(void);
static void    OnDebounceTimer(void *ctx);
static void    OnGapTimer(void *ctx);
static void    OnDayTimer(void *ctx);
static uint8_t DoorHall_ReadClosed(void);
static void    DoorHall_SendState(uint8_t closed);
static void    DoorHall_ClockSeed(void);
static uint8_t DoorHall_ClockIsSynced(void);
#if DOOR_TIME_SYNC_ENABLE
static void    DoorHall_TimeSyncTask(void);
static void    OnBootSyncTimer(void *ctx);
#endif /* DOOR_TIME_SYNC_ENABLE */

/* ============================================================================
 *  API PUBLICA
 * ==========================================================================*/
void DoorHall_Init(void)
{
  /* El pin HALL_DOOR (PA1) YA queda configurado por CubeMX en MX_GPIO_Init():
     entrada EXTI ambos flancos + pull-up, NVIC EXTI1 habilitado (ver .ioc).
     Aqui solo registramos tarea, timers y estado inicial. */
  DoorHall_ClockSeed();   /* pone el reloj en hora solo (hora de compilacion) */

  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_DoorTx), UTIL_SEQ_RFU, DoorHall_Task);
#if DOOR_TIME_SYNC_ENABLE
  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_DoorTimeSync), UTIL_SEQ_RFU,
                   DoorHall_TimeSyncTask);
  UTIL_TIMER_Create(&BootSyncTimer, DOOR_TIME_BOOT_SYNC_MS, UTIL_TIMER_ONESHOT,
                    OnBootSyncTimer, NULL);
  UTIL_TIMER_Start(&BootSyncTimer);
#endif /* DOOR_TIME_SYNC_ENABLE */

  UTIL_TIMER_Create(&DebounceTimer, DOOR_DEBOUNCE_MS,   UTIL_TIMER_ONESHOT,
                    OnDebounceTimer, NULL);
  UTIL_TIMER_Create(&GapTimer,      DOOR_TX_MIN_GAP_MS, UTIL_TIMER_ONESHOT,
                    OnGapTimer, NULL);
  UTIL_TIMER_Create(&DayTimer,      DOOR_MS_PER_DAY,    UTIL_TIMER_PERIODIC,
                    OnDayTimer, NULL);
  UTIL_TIMER_Start(&DayTimer);

  uint8_t closed = DoorHall_ReadClosed();

  APP_PPRINTF("\r\nDOOR HALL APP LISTA:\r\n");
  APP_PPRINTF("  Sensor Hall omnipolar en PA1 (HALL_DOOR, EXTI1 ambos flancos)\r\n");
  APP_PPRINTF("  B2 integrado = simulador (presionado => puerta CERRADA)\r\n");
  APP_PPRINTF("  Debounce %u ms | gap %u ms | tope %u/dia\r\n",
              (unsigned)DOOR_DEBOUNCE_MS, (unsigned)DOOR_TX_MIN_GAP_MS,
              (unsigned)DOOR_MAX_TX_PER_DAY);
#if DOOR_TIME_SYNC_ENABLE
  APP_PPRINTF("  Hora por downlink: al boot + re-sync cada %u dias\r\n",
              (unsigned)DOOR_TIME_RESYNC_DAYS);
#endif /* DOOR_TIME_SYNC_ENABLE */
  APP_PPRINTF("  Estado inicial: puerta %s\r\n\r\n",
              closed ? "CERRADA (iman presente)" : "ABIERTA (sin iman)");

#if DOOR_TX_STARTUP_STATE
  /* Reporta el estado inicial como primer uplink. */
  UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_DoorTx), CFG_SEQ_Prio_0);
#else
  /* No transmite al arrancar: adopta el estado actual como referencia. */
  lastTxState = closed;
#endif
}

void DoorHall_HandleEXTI(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == HALL_DOOR_Pin)
  {
    /* Cada flanco re-arma el debounce: la tarea corre solo cuando el nivel
       lleva DOOR_DEBOUNCE_MS estable (rebotes/vibracion colapsan aqui). */
    UTIL_TIMER_Stop(&DebounceTimer);
    UTIL_TIMER_Start(&DebounceTimer);
  }
}

/* ============================================================================
 *  LOGICA INTERNA
 * ==========================================================================*/

/**
  * @brief  Siembra AUTOMATICA del reloj con la hora de compilacion (__TIME__).
  *         Solo actua si el reloj no ha sido puesto en hora (Seconds pequeno =
  *         viene corriendo desde el reset). Un ajuste previo via AT$TIME o una
  *         siembra anterior (backup domain del RTC) NO se pisa: ese reloj ya
  *         viene contando bien.
  */
static void DoorHall_ClockSeed(void)
{
#if DOOR_CLOCK_SEED_BUILD_TIME
  SysTime_t now = SysTimeGet();

  if (now.Seconds < (2UL * 86400UL))   /* reloj nunca puesto en hora */
  {
    static const char bt[] = __TIME__;             /* "HH:MM:SS" del build */
    uint32_t hh = (uint32_t)(bt[0] - '0') * 10U + (uint32_t)(bt[1] - '0');
    uint32_t mm = (uint32_t)(bt[3] - '0') * 10U + (uint32_t)(bt[4] - '0');
    uint32_t ss = (uint32_t)(bt[6] - '0') * 10U + (uint32_t)(bt[7] - '0');

    SysTime_t seed;
    seed.Seconds    = DOOR_CLOCK_EPOCH_BASE + (hh * 3600U + mm * 60U + ss);
    seed.SubSeconds = 0;
    SysTimeSet(seed);

    APP_PPRINTF("  Reloj sembrado con hora de compilacion: %02u:%02u:%02u"
                " (afinar con AT$TIME=HH:MM:SS)\r\n",
                (unsigned)hh, (unsigned)mm, (unsigned)ss);
  }
#endif /* DOOR_CLOCK_SEED_BUILD_TIME */
}

static void OnDebounceTimer(void *ctx)
{
  (void)ctx;
  UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_DoorTx), CFG_SEQ_Prio_0);
}

static void OnGapTimer(void *ctx)
{
  (void)ctx;
  gapLocked = 0;
  /* Re-chequeo: si durante la ventana hubo mas cambios, reporta el estado
     FINAL (una rafaga de N eventos termina en 1 solo uplink coherente). */
  UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_DoorTx), CFG_SEQ_Prio_0);
}

static void OnDayTimer(void *ctx)
{
  (void)ctx;
  txToday = 0;
#if DOOR_TIME_SYNC_ENABLE
  /* Reintento diario si nunca ha sincronizado; re-sync periodico por deriva
     si ya lo esta. En ambos casos: maximo 1 downlink al dia. */
  daysSinceSync++;
  if (!DoorHall_ClockIsSynced() || (daysSinceSync >= DOOR_TIME_RESYNC_DAYS))
  {
    UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_DoorTimeSync), CFG_SEQ_Prio_0);
  }
#endif /* DOOR_TIME_SYNC_ENABLE */
}

#if DOOR_TIME_SYNC_ENABLE
static void OnBootSyncTimer(void *ctx)
{
  (void)ctx;
  if (!DoorHall_ClockIsSynced())
  {
    UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_DoorTimeSync), CFG_SEQ_Prio_0);
  }
}
#endif /* DOOR_TIME_SYNC_ENABLE */

/**
  * @brief  1 = el reloj tiene fecha/hora REAL (epoch >= 2025, solo lo produce
  *         el downlink del backend). La siembra de compilacion y AT$TIME usan
  *         la base epoch-2000 y cuentan como best-effort (bit0 = 0).
  */
static uint8_t DoorHall_ClockIsSynced(void)
{
  return (SysTimeGet().Seconds >= (uint32_t)DOOR_CLOCK_REAL_EPOCH_MIN) ? 1U : 0U;
}

#if DOOR_TIME_SYNC_ENABLE
/**
  * @brief  Pide la hora al backend: uplink 0xF0 con initiate_downlink_flag.
  *         Bloquea ~30-45 s (ventanas RX de Sigfox) dentro del sequencer.
  *         Respuesta esperada (8 bytes): [epoch UTC uint32 BE][4 reservados].
  */
static void DoorHall_TimeSyncTask(void)
{
  if ((DOOR_MAX_TX_PER_DAY != 0U) && (txToday >= DOOR_MAX_TX_PER_DAY))
  {
    return;                          /* presupuesto agotado: se difiere        */
  }

  uint8_t req[1] = { DOOR_TIME_REQ_BYTE };
  uint8_t dl_msg[8] = { 0 };

  APP_PPRINTF("[DOOR] >> Pidiendo hora por downlink (ventana RX ~30 s)...\r\n");
  txToday++;

  sfx_error_t err = SIGFOX_API_send_frame((sfx_u8 *)req, 1, dl_msg, 1, SFX_TRUE);

  if (err == SFX_ERR_NONE)
  {
    uint32_t epochUtc = ((uint32_t)dl_msg[0] << 24) | ((uint32_t)dl_msg[1] << 16)
                      | ((uint32_t)dl_msg[2] << 8)  |  (uint32_t)dl_msg[3];

    if (epochUtc >= (uint32_t)DOOR_CLOCK_REAL_EPOCH_MIN)
    {
      SysTime_t newTime;
      newTime.Seconds    = (uint32_t)((int64_t)epochUtc + DOOR_TZ_OFFSET_S);
      newTime.SubSeconds = 0;
      SysTimeSet(newTime);
      daysSinceSync = 0;

      uint32_t daySecs = (uint32_t)(newTime.Seconds % 86400UL);
      APP_PPRINTF("[DOOR] << Reloj sincronizado por downlink: %02u:%02u:%02u"
                  " (local, UTC%+ld h)\r\n\r\n",
                  (unsigned)(daySecs / 3600UL),
                  (unsigned)((daySecs % 3600UL) / 60UL),
                  (unsigned)(daySecs % 60UL),
                  (long)(DOOR_TZ_OFFSET_S / 3600L));
    }
    else
    {
      APP_PPRINTF("[DOOR] << Downlink recibido pero epoch invalido"
                  " (%02X%02X%02X%02X); se conserva la hora actual\r\n\r\n",
                  dl_msg[0], dl_msg[1], dl_msg[2], dl_msg[3]);
    }
  }
  else
  {
    APP_PPRINTF("[DOOR] << Sin respuesta de downlink (err 0x%04X);"
                " reintento en el ciclo de 24 h\r\n\r\n", err);
  }
}
#endif /* DOOR_TIME_SYNC_ENABLE */

/**
  * @brief  Lee el pin y regresa 1 = puerta CERRADA (iman presente).
  */
static uint8_t DoorHall_ReadClosed(void)
{
  GPIO_PinState lvl = HAL_GPIO_ReadPin(HALL_DOOR_GPIO_Port, HALL_DOOR_Pin);
#if DOOR_ACTIVE_LOW
  return (lvl == GPIO_PIN_RESET) ? 1U : 0U;
#else
  return (lvl == GPIO_PIN_SET) ? 1U : 0U;
#endif
}

static void DoorHall_SendState(uint8_t closed)
{
  uint8_t payload[6];
  uint8_t dl_msg[8] = { 0 };

  /* Hora del evento (HH:MM:SS) desde el RTC. Sin ajuste externo el reloj
     arranca en 00:00:00 al reset => equivale al uptime del equipo. */
  SysTime_t now = SysTimeGet();
  uint32_t daySecs = (uint32_t)(now.Seconds % 86400UL);
  uint8_t  hh = (uint8_t)(daySecs / 3600UL);
  uint8_t  mm = (uint8_t)((daySecs % 3600UL) / 60UL);
  uint8_t  ss = (uint8_t)(daySecs % 60UL);

  payload[0] = (uint8_t)((closed ? DOOR_PL_CLOSED : DOOR_PL_OPEN)      /* bit1 */
                         | (DoorHall_ClockIsSynced() ? 0x01U : 0x00U)); /* bit0 */
  payload[1] = hh;
  payload[2] = mm;
  payload[3] = ss;
  payload[4] = (uint8_t)(eventCount >> 8);
  payload[5] = (uint8_t)(eventCount & 0xFFU);

  APP_PPRINTF("[DOOR]    payload: %02X %02X %02X %02X %02X %02X (hora %02u:%02u:%02u)\r\n",
              payload[0], payload[1], payload[2], payload[3], payload[4], payload[5],
              (unsigned)hh, (unsigned)mm, (unsigned)ss);

  sfx_error_t err = SIGFOX_API_send_frame((sfx_u8 *)payload, 6,
                                          dl_msg, 1, SFX_FALSE);
  if (err == SFX_ERR_NONE)
  {
    APP_PPRINTF("[DOOR] << TX OK\r\n\r\n");
  }
  else
  {
    APP_PPRINTF("[DOOR] << TX ERROR 0x%04X\r\n\r\n", err);
  }
}

static void DoorHall_Task(void)
{
  uint8_t closed = DoorHall_ReadClosed();

  if (closed == lastTxState)
  {
    return;                         /* sin cambio real (rebote colapsado)     */
  }

  if (gapLocked)
  {
    /* Dentro de la ventana minima: OnGapTimer re-agendara este chequeo y el
       estado final se reportara entonces. */
    APP_PPRINTF("[DOOR] cambio a %s en ventana de %u ms, pendiente...\r\n",
                closed ? "CERRADA" : "ABIERTA", (unsigned)DOOR_TX_MIN_GAP_MS);
    return;
  }

  if ((DOOR_MAX_TX_PER_DAY != 0U) && (txToday >= DOOR_MAX_TX_PER_DAY))
  {
    /* Tope diario alcanzado: adopta el estado para no acumular un TX viejo. */
    APP_PPRINTF("[DOOR] tope diario (%u) alcanzado; evento %s NO transmitido\r\n",
                (unsigned)DOOR_MAX_TX_PER_DAY, closed ? "CIERRE" : "APERTURA");
    lastTxState = closed;
    return;
  }

  eventCount++;
  txToday++;

  APP_PPRINTF("[DOOR] >> TX evento #%u: puerta %s (tx %u/%u hoy)\r\n",
              (unsigned)eventCount,
              closed ? "CERRADA" : "ABIERTA",
              (unsigned)txToday, (unsigned)DOOR_MAX_TX_PER_DAY);

  DoorHall_SendState(closed);

  lastTxState = closed;
  gapLocked = 1;
  UTIL_TIMER_Start(&GapTimer);
}

#endif /* USE_DOOR_APP */
