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
  ******************************************************************************
  * Fecha:   Junio 2026
  * Version: 1.0
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

/* === Parametros configurables === */
#define BTN_DEBOUNCE_MS         200U      /* Tiempo de debounce por software */
#define BTN_RATE_LIMIT_MS       30000U    /* Minimo entre dos uplinks (30 s) */
#define BTN_TX_REPLICAS         1U        /* Numero de replicas TX1/TX2/TX3 */
#define BTN_LED_OK_HOLD_MS      3000U     /* Tiempo que queda encendido el LED tras TX OK */
#define BTN_LED_ERR_HOLD_MS     3000U     /* Tiempo que queda encendido el LED tras TX ERROR */

/* === API publica === */

/**
  * @brief  Inicializa los handlers de los botones. Llamar desde Sigfox_Init()
  *         despues de que el stack Sigfox este abierto.
  *         Habilita NVIC para EXTI0/EXTI1/EXTI9_5 y registra la tarea en el
  *         sequencer de ST.
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
