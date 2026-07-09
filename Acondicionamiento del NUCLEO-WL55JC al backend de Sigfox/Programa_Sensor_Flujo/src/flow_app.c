/**
  ******************************************************************************
  * @file    flow_app.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Medicion de un sensor de FLUJO de efecto Hall (YF-S201, 1-30 L/min).
  *          Cuenta pulsos por EXTI y en una ventana de 1 s calcula frecuencia
  *          (Hz) -> caudal (L/min, K configurable) y volumen acumulado.
  *          Boton por EXTI (cuenta pulsaciones). Log por Vcom para caracterizar.
  ******************************************************************************
  * Empresa: 0G IoT Solutions (previamente WND Mexico) - https://0giotsolutions.com/
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "flow_app.h"
#include "app_features.h"

#if USE_FLOW_APP   /* ==== Modulo completo condicionado por feature flag ==== */

#include <stdint.h>
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "sys_app.h"
#if FLOW_TX_ENABLE
#include "sigfox_types.h"
#include "st_sigfox_api.h"
#endif

/* === Estado === */
static UTIL_TIMER_Object_t FlowTimer;
static volatile uint32_t   pulseCount   = 0;   /* pulsos en la ventana (ISR)   */
static volatile uint32_t   btnPresses   = 0;   /* pulsaciones de boton (ISR)   */
static volatile uint32_t   lastBtnTick  = 0;   /* debounce del boton (ISR)     */
static uint32_t            totalPulses  = 0;   /* pulsos acumulados            */
static uint32_t            lastTxPulses = 0;   /* totalPulses en el ultimo TX  */
static volatile uint8_t    btnUplinkReq = 0;   /* boton pide enviar total YA   */

/* === Prototipos === */
static void Flow_Task(void);
static void OnFlowTimer(void *ctx);

/* ============================================================================
 *  API PUBLICA
 * ==========================================================================*/
void Flow_Init(void)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Senal de flujo: pulsos -> interrupcion en flanco de subida.
     Open-collector: pull-up (externo 10k a 3V3; interno de respaldo). */
  g.Pin  = FLOW_Pin;
  g.Mode = GPIO_MODE_IT_RISING;
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FLOW_GPIO_Port, &g);

  /* Boton activo en bajo: interrupcion en flanco de bajada. */
  g.Pin  = FLOW_BTN_Pin;
  g.Mode = GPIO_MODE_IT_FALLING;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(FLOW_BTN_GPIO_Port, &g);

  /* Ambos pines (PB8=linea 8, PB5=linea 5) caen en EXTI9_5. Prioridad baja
     para no estorbar al radio Sigfox. */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_FlowTx), UTIL_SEQ_RFU, Flow_Task);

  UTIL_TIMER_Create(&FlowTimer, FLOW_WINDOW_MS, UTIL_TIMER_PERIODIC,
                    OnFlowTimer, NULL);
  UTIL_TIMER_Start(&FlowTimer);

  APP_PPRINTF("\r\nFLOW APP LISTA (caracterizacion):\r\n");
  APP_PPRINTF("  Senal en PB8 (D5), boton en PB5 (D4) | ventana %u ms\r\n",
              (unsigned)FLOW_WINDOW_MS);
  APP_PPRINTF("  K = %u pulsos/litro (F=7.5*Q tipico) | sopla la turbina para probar\r\n\r\n",
              (unsigned)FLOW_PULSES_PER_LITER);
}

/* ============================================================================
 *  INTERRUPCIONES (EXTI 5..9): pulsos de flujo + boton
 * ==========================================================================*/
void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(FLOW_Pin);
  HAL_GPIO_EXTI_IRQHandler(FLOW_BTN_Pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == FLOW_Pin)
  {
    pulseCount++;             /* un pulso mas de la turbina */
  }
  else if (GPIO_Pin == FLOW_BTN_Pin)
  {
    uint32_t now = HAL_GetTick();
    if ((now - lastBtnTick) >= FLOW_BTN_DEBOUNCE_MS)  /* debounce */
    {
      lastBtnTick = now;
      btnPresses++;
      btnUplinkReq = 1;         /* pide enviar el total actual (a demanda) */
    }
  }
}

/* ============================================================================
 *  LOGICA
 * ==========================================================================*/
static void OnFlowTimer(void *ctx)
{
  (void)ctx;
  UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_FlowTx), CFG_SEQ_Prio_0);
}

static void Flow_Task(void)
{
  uint32_t count, freq_hz, flow_mLmin, vol_mL, presses;

  /* Lee y reinicia el contador de pulsos de forma atomica. */
  __disable_irq();
  count = pulseCount;
  pulseCount = 0;
  __enable_irq();

  totalPulses += count;
  presses = btnPresses;

  /* Frecuencia (Hz) en la ventana. Con ventana=1000ms, freq = count. */
  freq_hz = (count * 1000UL) / FLOW_WINDOW_MS;

  /* Caudal: F(Hz) -> mL/min.  mL/min = freq * 60000 / (pulsos/litro). */
  flow_mLmin = (freq_hz * 60000UL) / FLOW_PULSES_PER_LITER;

  /* Volumen acumulado en mL = totalPulses * 1000 / (pulsos/litro). */
  vol_mL = (uint32_t)(((uint64_t)totalPulses * 1000ULL) / FLOW_PULSES_PER_LITER);

  {
    /* Diagnostico del boton: nivel crudo de PB5 (idle deberia ser 1 con el
       pull-up; al presionar debe caer a 0). Si siempre marca 0 -> PB5 esta
       forzado a GND (cableado/conflicto). Si siempre 1 y no cae al presionar
       -> el boton no llega a PB5. */
    uint8_t btnLvl = (HAL_GPIO_ReadPin(FLOW_BTN_GPIO_Port, FLOW_BTN_Pin)
                      == GPIO_PIN_SET) ? 1U : 0U;
    APP_PPRINTF("[FLOW] f=%u Hz  Q=%u mL/min (%u.%02u L/min)  Vol=%u mL  pulsos=%u  btn=%u  PB5=%u\r\n",
                (unsigned)freq_hz,
                (unsigned)flow_mLmin,
                (unsigned)(flow_mLmin / 1000U), (unsigned)((flow_mLmin % 1000U) / 10U),
                (unsigned)vol_mL,
                (unsigned)totalPulses,
                (unsigned)presses,
                (unsigned)btnLvl);
  }

#if FLOW_TX_ENABLE
  /* Totalizador: uplink cada FLOW_UPLINK_LITERS acumulados, O a demanda por
     boton. Payload = volumen TOTAL acumulado (mL) + caudal actual. El total
     NUNCA se reinicia; el boton solo fuerza un envio del total ACTUAL. */
  {
    uint8_t     doTx   = 0;
    const char *motivo = "";

    if ((totalPulses - lastTxPulses) >=
        ((uint32_t)FLOW_UPLINK_LITERS * FLOW_PULSES_PER_LITER))
    {
      doTx = 1; motivo = "10 L";
    }

    if (btnUplinkReq)
    {
      btnUplinkReq = 0;
      APP_PPRINTF("[FLOW] BOTON presionado (uplink manual)\r\n");
#if FLOW_BTN_TX
      doTx = 1; motivo = "boton";
#endif
    }

    if (doTx)
    {
      uint8_t payload[6];
      uint8_t dl[8] = { 0 };
      sfx_error_t err;

      btnUplinkReq = 0;

      /* vol_mL = volumen TOTAL acumulado (uint32, big-endian). */
      payload[0] = (uint8_t)(vol_mL >> 24);
      payload[1] = (uint8_t)(vol_mL >> 16);
      payload[2] = (uint8_t)(vol_mL >> 8);
      payload[3] = (uint8_t)(vol_mL);
      payload[4] = (uint8_t)(flow_mLmin >> 8);
      payload[5] = (uint8_t)(flow_mLmin);

      APP_PPRINTF("[FLOW] >> UPLINK (%s) total=%u mL (%u.%03u L)  caudal=%u mL/min\r\n",
                  motivo, (unsigned)vol_mL,
                  (unsigned)(vol_mL / 1000U), (unsigned)(vol_mL % 1000U),
                  (unsigned)flow_mLmin);

      err = SIGFOX_API_send_frame((sfx_u8 *)payload, 6, dl, 1, SFX_FALSE);
      if (err == SFX_ERR_NONE)
      {
        APP_PPRINTF("[FLOW] << TX OK\r\n");
      }
      else
      {
        APP_PPRINTF("[FLOW] << TX ERROR 0x%04X\r\n", err);
      }

      /* Re-marca la ventana de 10 L desde el punto actual (aplica para ambos
         motivos, asi el boton no dispara un uplink duplicado justo antes del 10L). */
      lastTxPulses = totalPulses;
    }
  }
#endif
}

#endif /* USE_FLOW_APP */
