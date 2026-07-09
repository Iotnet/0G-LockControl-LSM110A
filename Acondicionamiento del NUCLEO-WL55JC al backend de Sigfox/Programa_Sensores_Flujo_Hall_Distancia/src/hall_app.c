/**
  ******************************************************************************
  * @file    hall_app.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Sensor Hall (magnetico, tipo puerta) + boton externo, ambos con
  *          filtro RC (10k + 1nF). Sondeo periodico con debounce por software;
  *          envio Sigfox SOLO al cambiar el estado (evento). Tope diario.
  *
  *          Cableado tipico (filtro RC): pin -- 10k --> 3V3, pin -- 1nF --> GND,
  *          y el Hall/boton cierran a GND. => en reposo ALTO, ACTIVO en BAJO.
  *
  *          Payload (1 byte): bit0 = Hall activo (iman presente / puerta cerrada)
  *                            bit1 = boton presionado
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hall_app.h"
#include "app_features.h"

#if USE_HALL_APP   /* ==== Modulo completo condicionado por feature flag ==== */

#include <stdint.h>
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "sys_app.h"
#include "sigfox_types.h"
#include "st_sigfox_api.h"

/* === Estado === */
static UTIL_TIMER_Object_t HallTimer;
static uint8_t  lastRaw     = 0xFF;   /* ultima lectura cruda (para debounce)  */
static uint8_t  stableFlags = 0xFF;   /* estado confirmado (2 lecturas iguales)*/
static uint8_t  lastTxFlags = 0xFF;   /* estado del ultimo TX                  */
static uint32_t sampleCount = 0;
static uint32_t dayStart    = 0;
static uint32_t txToday     = 0;

#define SAMPLES_PER_DAY  ((24UL * 60UL * 60UL * 1000UL) / HALL_SAMPLE_MS)

/* === Prototipos === */
static void Hall_Task(void);
static void OnHallTimer(void *ctx);
static uint8_t Hall_ReadFlags(void);
static void Hall_SendPayload(uint8_t flags);

/* ============================================================================
 *  API PUBLICA
 * ==========================================================================*/
void Hall_Init(void)
{
  GPIO_InitTypeDef g = {0};

  /* Pines como entrada. El pull-up lo da la 10k externa del filtro RC, por eso
     NOPULL (para no alterar la constante RC con el pull-up interno). */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;    /* respaldo: si falta el pull-up externo, el pin no flota */
  g.Speed = GPIO_SPEED_FREQ_LOW;

  g.Pin = HALL_Pin;    HAL_GPIO_Init(HALL_GPIO_Port, &g);
  g.Pin = BTN_EXT_Pin; HAL_GPIO_Init(BTN_EXT_GPIO_Port, &g);

  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_HallTx), UTIL_SEQ_RFU, Hall_Task);

  UTIL_TIMER_Create(&HallTimer, HALL_SAMPLE_MS, UTIL_TIMER_PERIODIC,
                    OnHallTimer, NULL);
  UTIL_TIMER_Start(&HallTimer);

  APP_PPRINTF("\r\nHALL APP LISTA:\r\n");
  APP_PPRINTF("  Hall en P%c%u, boton en P%c%u | Sondeo %u ms\r\n",
              'B', 8U, 'B', 5U, (unsigned)HALL_SAMPLE_MS);
  APP_PPRINTF("  TX SOLO por evento (cambio de estado) | tope %u/dia\r\n\r\n",
              (unsigned)HALL_MAX_TX_PER_DAY);
}

/* ============================================================================
 *  LOGICA INTERNA
 * ==========================================================================*/
static void OnHallTimer(void *ctx)
{
  (void)ctx;
  UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_HallTx), CFG_SEQ_Prio_0);
}

/**
  * @brief  Lee ambas entradas y arma el byte de flags (bit0=Hall, bit1=boton).
  */
static uint8_t Hall_ReadFlags(void)
{
  GPIO_PinState hall = HAL_GPIO_ReadPin(HALL_GPIO_Port, HALL_Pin);
  GPIO_PinState btn  = HAL_GPIO_ReadPin(BTN_EXT_GPIO_Port, BTN_EXT_Pin);

  uint8_t hall_active = (HALL_ACTIVE_LOW ? (hall == GPIO_PIN_RESET)
                                         : (hall == GPIO_PIN_SET));
  uint8_t btn_active  = (BTN_ACTIVE_LOW  ? (btn == GPIO_PIN_RESET)
                                         : (btn == GPIO_PIN_SET));

  return (uint8_t)((hall_active ? 0x01U : 0U) | (btn_active ? 0x02U : 0U));
}

static void Hall_SendPayload(uint8_t flags)
{
  uint8_t payload[1] = { flags };
  uint8_t dl_msg[8]  = { 0 };
  sfx_error_t err = SIGFOX_API_send_frame((sfx_u8 *)payload, 1,
                                          dl_msg, 1, SFX_FALSE);
  if (err == SFX_ERR_NONE)
  {
    APP_PPRINTF("[HALL] << TX OK\r\n\r\n");
  }
  else
  {
    APP_PPRINTF("[HALL] << TX ERROR 0x%04X\r\n\r\n", err);
  }
}

static void Hall_Task(void)
{
  uint8_t raw = Hall_ReadFlags();

  /* Debounce: acepta un estado nuevo solo si 2 lecturas seguidas coinciden. */
  if (raw == lastRaw && raw != stableFlags)
  {
    stableFlags = raw;
  }
  lastRaw = raw;

  sampleCount++;
  if ((sampleCount - dayStart) >= SAMPLES_PER_DAY)
  {
    txToday = 0;
    dayStart = sampleCount;
  }

  /* Imprime el estado SOLO cuando cambia (no cada muestra). */
  if (stableFlags != lastTxFlags)
  {
    APP_PPRINTF("[HALL] Hall=%s  Boton=%s  flags=0x%02X\r\n",
                (stableFlags & 0x01) ? "CERRADA/IMAN" : "ABIERTA/SIN",
                (stableFlags & 0x02) ? "PRESIONADO" : "suelto",
                (unsigned)stableFlags);
  }

  /* Transmite SOLO al cambiar el estado confirmado, respetando el tope diario */
  if (stableFlags != lastTxFlags)
  {
    uint8_t budgetOk = (HALL_MAX_TX_PER_DAY == 0U) || (txToday < HALL_MAX_TX_PER_DAY);
    if (budgetOk)
    {
      APP_PPRINTF("[HALL] >> TX evento: Hall=%s Boton=%s (0x%02X, tx %u/%u)\r\n",
                  (stableFlags & 0x01) ? "CERRADA" : "ABIERTA",
                  (stableFlags & 0x02) ? "SI" : "no",
                  (unsigned)stableFlags,
                  (unsigned)txToday, (unsigned)HALL_MAX_TX_PER_DAY);
      Hall_SendPayload(stableFlags);
      txToday++;
    }
    lastTxFlags = stableFlags;   /* acepta el estado aunque el tope bloquee */
  }
}

#endif /* USE_HALL_APP */
