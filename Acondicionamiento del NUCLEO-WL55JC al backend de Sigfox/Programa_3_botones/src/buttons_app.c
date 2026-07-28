/**
  ******************************************************************************
  * @file    buttons_app.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Logica de envios Sigfox disparados por los push-buttons
  *          B1, B2 y B3 del NUCLEO-WL55JC.
  *
  *          Flujo: ISR del boton -> debounce -> set task en sequencer
  *                 -> tarea ejecuta SIGFOX_API_send_frame con el payload
  *                    correspondiente -> LEDs indican el estado.
  *
  *          B1 (PA0)  --> "Hola"   (hex 48 6F 6C 61)
  *          B2 (PA1)  --> "Mundo"  (hex 4D 75 6E 64 6F)
  *          B3 (PC6)  --> "0G"     (hex 30 47)
  *
  *          LEDs:  LD3 (rojo)  -> TX en curso
  *                 LD2 (verde) -> TX OK
  *                 LD1 (azul)  -> rate limit o error
  *
  *          La interfaz AT del firmware Sigfox_AT_Slave sigue activa en
  *          paralelo. Los botones son un canal adicional, no un reemplazo.
  *
  *          v1.1 - El debounce y el rate limit pasan de HAL_GetTick() a
  *          timers UTIL_TIMER (base RTC), que siguen contando en Stop2.
  *          Ver la nota completa en buttons_app.h.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.1
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "buttons_app.h"
#include <stdint.h>
#include "sigfox_types.h"
#include "st_sigfox_api.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "sys_app.h"

/* === Payloads (definidos como literales para que queden en flash) === */
static const uint8_t PAYLOAD_B1[] = { 0x48, 0x6F, 0x6C, 0x61 };          /* "Hola"  */
static const uint8_t PAYLOAD_B2[] = { 0x4D, 0x75, 0x6E, 0x64, 0x6F };    /* "Mundo" */
static const uint8_t PAYLOAD_B3[] = { 0x30, 0x47 };                       /* "0G"    */

/* === Identificador interno de los botones === */
typedef enum {
  BTN_NONE = 0,
  BTN_B1   = 1,
  BTN_B2   = 2,
  BTN_B3   = 3
} ButtonId_t;

/* === Estado ===
   Los dos "candados" se abren desde callbacks de UTIL_TIMER, que corren sobre
   el RTC y por tanto vencen aunque el MCU haya estado dormido en Stop2. */
static volatile ButtonId_t pendingButton   = BTN_NONE;  /* Set en ISR, leido en tarea */
static volatile uint8_t    debounceLocked  = 0U;        /* 1 = dentro del anti-rebote */

static UTIL_TIMER_Object_t DebounceTimer;               /* one-shot BTN_DEBOUNCE_MS   */
#if (BTN_RATE_LIMIT_MS > 0U)
static volatile uint8_t    gapLocked       = 0U;        /* 1 = dentro del rate limit  */
static UTIL_TIMER_Object_t GapTimer;                    /* one-shot BTN_RATE_LIMIT_MS */
#endif

/* === Macros de LED ===
   LD1 (azul)  -> PB15 -> indicador de rate-limit / error
   LD2 (verde) -> PB9  -> indicador de TX OK
   LD3 (rojo)  -> PB11 -> indicador de TX en curso */
#define LED_BUSY_ON()    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET)
#define LED_BUSY_OFF()   HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET)
#define LED_OK_ON()      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET)
#define LED_OK_OFF()     HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)
#define LED_ERR_ON()     HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED_ERR_OFF()    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)

/* === Prototipos privados === */
static void Buttons_Process(void);
static void Buttons_SendPayload(ButtonId_t button);
static void Buttons_BlinkBlue(uint8_t times);
static void OnDebounceTimer(void *ctx);
#if (BTN_RATE_LIMIT_MS > 0U)
static void OnGapTimer(void *ctx);
#endif

/* ============================================================================
 *  API PUBLICA
 * ============================================================================
 */

void Buttons_Init(void)
{
  /* Apaga los 3 LEDs al arrancar */
  LED_BUSY_OFF();
  LED_OK_OFF();
  LED_ERR_OFF();

  /* Habilita NVIC para los 3 lineas EXTI:
     - PA0 (B1) -> EXTI0
     - PA1 (B2) -> EXTI1
     - PC6 (B3) -> EXTI9_5 (linea 6 pertenece al grupo [5:9])
     Prioridad baja (5) para no estorbar al radio de Sigfox. */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* Timers de base RTC (siguen corriendo en Stop2, a diferencia de SysTick).
     Se crean detenidos: se arrancan al vuelo cuando hacen falta. */
  UTIL_TIMER_Create(&DebounceTimer, BTN_DEBOUNCE_MS, UTIL_TIMER_ONESHOT,
                    OnDebounceTimer, NULL);
#if (BTN_RATE_LIMIT_MS > 0U)
  UTIL_TIMER_Create(&GapTimer, BTN_RATE_LIMIT_MS, UTIL_TIMER_ONESHOT,
                    OnGapTimer, NULL);
#endif

  /* Registra la tarea en el sequencer. Esta tarea sera disparada por la ISR. */
  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_ButtonTx), UTIL_SEQ_RFU, Buttons_Process);

  APP_PPRINTF("\r\nBOTONES LISTOS:\r\n");
  APP_PPRINTF("  B1 (PA0) --> Hola  (48 6F 6C 61)\r\n");
  APP_PPRINTF("  B2 (PA1) --> Mundo (4D 75 6E 64 6F)\r\n");
  APP_PPRINTF("  B3 (PC6) --> 0G    (30 47)\r\n");
#if (BTN_RATE_LIMIT_MS > 0U)
  APP_PPRINTF("  Debounce: %u ms  |  Rate limit: %u s  (base RTC)\r\n\r\n",
              (unsigned)BTN_DEBOUNCE_MS,
              (unsigned)(BTN_RATE_LIMIT_MS / 1000U));
#else
  APP_PPRINTF("  Debounce: %u ms  |  Rate limit: DESHABILITADO (modo GATE 2)\r\n\r\n",
              (unsigned)BTN_DEBOUNCE_MS);
#endif
}

void Buttons_HandleEXTI(uint16_t GPIO_Pin)
{
  /* Debounce de software: mientras el candado este cerrado se ignora todo
     (evita el rebote mecanico del boton). Lo abre OnDebounceTimer().
     A diferencia de la v1.0 no hay ventana muerta al arrancar ni queda
     bloqueado cuando el MCU duerme: el timer corre sobre el RTC. */
  if (debounceLocked != 0U) {
    return;
  }

  /* Mapea pin a boton */
  ButtonId_t btn = BTN_NONE;
  if (GPIO_Pin == BUT1_Pin) {
    btn = BTN_B1;
  } else if (GPIO_Pin == BUT2_Pin) {
    btn = BTN_B2;
  } else if (GPIO_Pin == BUT3_Pin) {
    btn = BTN_B3;
  }

  if (btn == BTN_NONE) {
    return;
  }

  debounceLocked = 1U;
  UTIL_TIMER_Start(&DebounceTimer);

  /* Si ya hay otro boton pendiente, lo descartamos (evita encolar TX dobles).
     El primer click gana. */
  if (pendingButton == BTN_NONE) {
    pendingButton = btn;
    UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_ButtonTx), CFG_SEQ_Prio_0);
  }
}

/* ============================================================================
 *  LOGICA INTERNA
 * ============================================================================
 */

/**
  * @brief  Callback del timer de debounce: reabre la ventana de pulsaciones.
  *         Corre en contexto de IRQ del RTC -> solo toca la bandera.
  */
static void OnDebounceTimer(void *ctx)
{
  (void)ctx;
  debounceLocked = 0U;
}

#if (BTN_RATE_LIMIT_MS > 0U)
/**
  * @brief  Callback del timer de rate limit: reabre la ventana de uplinks.
  *         Corre en contexto de IRQ del RTC -> solo toca la bandera.
  */
static void OnGapTimer(void *ctx)
{
  (void)ctx;
  gapLocked = 0U;
}
#endif

/**
  * @brief  Tarea del sequencer disparada por la ISR.
  *         Verifica rate limit y, si OK, llama Buttons_SendPayload().
  */
static void Buttons_Process(void)
{
  ButtonId_t btn = pendingButton;
  pendingButton = BTN_NONE;     /* Listo para aceptar el siguiente click */

  if (btn == BTN_NONE) {
    return;
  }

#if (BTN_RATE_LIMIT_MS > 0U)
  /* Rate limit: el candado esta cerrado desde el ultimo TX exitoso hasta que
     vence GapTimer (BTN_RATE_LIMIT_MS de reloj real, dormido o despierto). */
  if (gapLocked != 0U) {
    APP_PPRINTF("[BTN] Rate limit activo (%u s desde el ultimo TX). Uplink rechazado.\r\n",
                (unsigned)(BTN_RATE_LIMIT_MS / 1000U));
    Buttons_BlinkBlue(6);   /* 6 parpadeos rapidos = uplink rechazado */
    return;
  }
#endif

  Buttons_SendPayload(btn);
}

/**
  * @brief  Realiza el envio Sigfox del payload asociado al boton presionado.
  */
static void Buttons_SendPayload(ButtonId_t button)
{
  const uint8_t *payload = NULL;
  uint8_t        length  = 0U;
  const char    *label   = "";

  switch (button) {
    case BTN_B1:
      payload = PAYLOAD_B1;
      length  = (uint8_t)sizeof(PAYLOAD_B1);
      label   = "B1 = Hola";
      break;
    case BTN_B2:
      payload = PAYLOAD_B2;
      length  = (uint8_t)sizeof(PAYLOAD_B2);
      label   = "B2 = Mundo";
      break;
    case BTN_B3:
      payload = PAYLOAD_B3;
      length  = (uint8_t)sizeof(PAYLOAD_B3);
      label   = "B3 = 0G";
      break;
    default:
      return;
  }

  APP_PPRINTF("\r\n[BTN] >> TX %s (%u bytes)\r\n", label, length);

  /* Senalizacion visual: LED rojo encendido durante la transmision.
     Se puede desactivar (BTN_LED_TX_INDICATOR 0) para que el dip medido en el
     GATE 2 sea el del radio solo, sin los ~2-3 mA del LED. */
  LED_OK_OFF();
  LED_ERR_OFF();
#if (BTN_LED_TX_INDICATOR != 0)
  LED_BUSY_ON();
#endif

  uint8_t dl_msg[8] = { 0 };
  sfx_error_t err = SIGFOX_API_send_frame((sfx_u8 *)payload,
                                          (sfx_u8)length,
                                          dl_msg,
                                          (sfx_u8)BTN_TX_REPLICAS,
                                          SFX_FALSE);

  LED_BUSY_OFF();

  if (err == SFX_ERR_NONE) {
    LED_OK_ON();
#if (BTN_RATE_LIMIT_MS > 0U)
    gapLocked = 1U;                 /* Arranca el rate-limit (base RTC) */
    UTIL_TIMER_Start(&GapTimer);
#endif
    APP_PPRINTF("[BTN] << TX OK\r\n\r\n");
#if (BTN_LED_OK_HOLD_MS > 0U)
    HAL_Delay(BTN_LED_OK_HOLD_MS);
#endif
    LED_OK_OFF();
  } else {
    LED_ERR_ON();
    APP_PPRINTF("[BTN] << TX ERROR 0x%04X\r\n\r\n", err);
#if (BTN_LED_ERR_HOLD_MS > 0U)
    HAL_Delay(BTN_LED_ERR_HOLD_MS);
#endif
    LED_ERR_OFF();
  }
}

/**
  * @brief  Patron de parpadeo del LED azul para indicar rate-limit hit.
  */
static void Buttons_BlinkBlue(uint8_t times)
{
  for (uint8_t i = 0; i < times; i++) {
    LED_ERR_ON();
    HAL_Delay(80);
    LED_ERR_OFF();
    HAL_Delay(80);
  }
}
