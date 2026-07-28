/**
  ******************************************************************************
  * @file    buttons_app.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Cabecera para el manejo de los push-buttons B1/B2/B3
  *          que disparan envios Sigfox con payloads diferenciados.
  *
  *          B1 (PA0)  --> "Hola"   (hex 48 6F 6C 61, 4 bytes)
  *          B2 (PA1)  --> "Mundo"  (hex 4D 75 6E 64 6F, 5 bytes)
  *          B3 (PC6)  --> "0G"     (hex 30 47, 2 bytes)
  *
  *          Incluye debounce por software (200 ms), rate limiting
  *          entre uplinks (30 s) y feedback visual por LEDs.
  *
  *          v1.1 - El debounce y el rate limit se cuentan con UTIL_TIMER
  *          (base RTC) en lugar de HAL_GetTick(). Ver nota de abajo.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.1
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __BUTTONS_APP_H__
#define __BUTTONS_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ============================================================================
 *  POR QUE NO SE USA HAL_GetTick() PARA MEDIR EL TIEMPO (v1.1)
 * ============================================================================
 *  HAL_GetTick() cuenta sobre SysTick, y SysTick se DETIENE cuando el MCU
 *  entra en Stop2. Este firmware (Sigfox_AT_Slave con LOW_POWER_DISABLE=0)
 *  duerme en Stop2 en idle, asi que el contador de HAL practicamente se
 *  congela entre eventos: 30 s de reloj de pared pueden ser <1 s de ticks.
 *
 *  Consecuencia de la v1.0 (bug reproducido en GATE 2): alimentando la placa
 *  desde fuente/pila -- sin USB, que es cuando la placa SI llega a dormir --
 *  la ventana del rate limit nunca vencia y el firmware rechazaba todos los
 *  uplinks posteriores al primero: 6 parpadeos del LED azul y ningun TX. El
 *  mismo congelamiento afectaba al debounce, dejando los botones "muertos".
 *
 *  Los timers de UTIL_TIMER (stm32_timer + timer_if) corren sobre el RTC, que
 *  SI sigue contando en Stop2. Es el mismo patron que usa hall_door_app.c.
 * ==========================================================================*/

/* === Modo GATE 2 ============================================================
 *  1 = configuracion para la medicion del pulso TX (Hardware/validation/
 *      gate2-tx-pulse/): sin rate limit y sin holds de LED, de modo que cada
 *      pulsacion transmita y el riel vuelva a reposo en cuanto termina el TX
 *      (un LED encendido roba ~2-3 mA que retrasan la recarga del cap de
 *      470 uF entre disparos).
 *  0 = configuracion de demo normal (rate limit 30 s, holds de 3 s).
 *
 *  OJO: en modo GATE 2 cada pulsacion consume un token del contrato Sigfox.
 * ==========================================================================*/
#define BTN_GATE2_MODE          0

/* === Parametros configurables === */
#define BTN_DEBOUNCE_MS         200U      /* Ventana anti-rebote por software */
#define BTN_TX_REPLICAS         1U        /* Numero de replicas TX1/TX2/TX3 */

#if (BTN_GATE2_MODE != 0)
#define BTN_RATE_LIMIT_MS       0U        /* 0 = rate limit deshabilitado */
#define BTN_LED_OK_HOLD_MS      0U        /* 0 = apaga el LED de inmediato */
#define BTN_LED_ERR_HOLD_MS     0U
#else
#define BTN_RATE_LIMIT_MS       30000U    /* Minimo entre dos uplinks (30 s) */
#define BTN_LED_OK_HOLD_MS      3000U     /* Tiempo que queda encendido el LED tras TX OK */
#define BTN_LED_ERR_HOLD_MS     3000U     /* Tiempo que queda encendido el LED tras TX ERROR */
#endif

/* 1 = LED rojo encendido durante el TX. Es el unico feedback visual cuando no
   hay USB conectado, pero suma ~2-3 mA al consumo del pulso: para la corrida
   oficial del GATE 2 (dip del radio solo) ponerlo en 0 y disparar el
   osciloscopio por flanco de bajada. */
#define BTN_LED_TX_INDICATOR    1

/* === API publica === */

/**
  * @brief  Inicializa los handlers de los botones. Llamar desde Sigfox_Init()
  *         despues de que el stack Sigfox este abierto.
  *         Habilita NVIC para EXTI0/EXTI1/EXTI9_5, crea los timers RTC de
  *         debounce/rate-limit y registra la tarea en el sequencer de ST.
  */
void Buttons_Init(void);

/**
  * @brief  Procesa el evento EXTI de un boton. Llamado desde
  *         HAL_GPIO_EXTI_Callback() en stm32wlxx_it.c.
  * @param  GPIO_Pin pin que disparo la interrupcion (BUT1_Pin/BUT2_Pin/BUT3_Pin)
  */
void Buttons_HandleEXTI(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTONS_APP_H__ */
