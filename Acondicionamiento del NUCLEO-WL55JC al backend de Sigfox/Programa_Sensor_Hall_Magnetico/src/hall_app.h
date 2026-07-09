/**
  ******************************************************************************
  * @file    hall_app.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Lectura de un sensor Hall (efecto magnetico, tipo puerta) y un
  *          boton externo, ambos con filtro RC por hardware (10k + 1nF/"102").
  *          Envia por Sigfox SOLO cuando cambia el estado (evento), sin
  *          periodicidad. Mismo patron que distance_app (timer + task del
  *          sequencer + envio por cambio + tope diario).
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __HALL_APP_H__
#define __HALL_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ===================== Pines de entrada (CAMBIALOS a tu cableado) ============
   Ambos van con el filtro RC: 10k a 3V3 (pull-up) + 102 (1nF) a GND, y el
   Hall/boton cierran a GND -> pin en reposo ALTO, ACTIVO en BAJO.
     HALL  -> PB8 (Arduino D5)
     BOTON -> PB5 (Arduino D4)
   Confirma/ajusta estos pines ANTES de cablear. */
#define HALL_Pin            GPIO_PIN_8
#define HALL_GPIO_Port      GPIOB
#define BTN_EXT_Pin         GPIO_PIN_5
#define BTN_EXT_GPIO_Port   GPIOB

/* Nivel activo: 1 = activo en BAJO (pull-up + cierre a GND, lo normal con el
   filtro RC). Si tu Hall es push-pull activo en alto, pon 0. */
#define HALL_ACTIVE_LOW     1
#define BTN_ACTIVE_LOW      1

/* ===================== Cadencias / limites ================================== */
#define HALL_SAMPLE_MS      200U     /* sondeo cada 200 ms (capta el boton) */
#define HALL_DEBOUNCE_MS    50U      /* estado debe mantenerse 50 ms para valer */
#define HALL_MAX_TX_PER_DAY 100U     /* tope duro de uplinks/24h (0 = sin tope) */

/* ===================== API publica ========================================= */

/**
  * @brief  Configura los pines (entrada), registra la tarea de sondeo y arranca
  *         el timer. Llamar desde Sigfox_Init() bajo #if USE_HALL_APP.
  */
void Hall_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __HALL_APP_H__ */
