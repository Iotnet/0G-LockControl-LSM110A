/**
  ******************************************************************************
  * @file    flow_app.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Sensor de FLUJO de efecto Hall (tipo YF-S201, 1-30 L/min) + boton.
  *          La turbina genera un tren de pulsos cuya FRECUENCIA es proporcional
  *          al caudal. Se cuentan los pulsos por interrupcion (EXTI), y en una
  *          ventana se calcula Hz -> L/min (K configurable) + volumen acumulado.
  *          Pensado para CARACTERIZACION (log por Vcom).
  *
  *          Cableado:
  *            Senal (amarillo) -> D5 (PB8)  [open-collector: pull-up 10k a 3V3]
  *            VCC (rojo)        -> 5V
  *            GND (negro)       -> GND
  *            Boton             -> D4 (PB5)  [pull-up + cierre a GND]
  ******************************************************************************
  * Empresa: 0G IoT Solutions (previamente WND Mexico) - https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __FLOW_APP_H__
#define __FLOW_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ===================== Pines ================================================ */
#define FLOW_Pin              GPIO_PIN_8      /* Arduino D5 - senal de pulsos  */
#define FLOW_GPIO_Port        GPIOB
#define FLOW_BTN_Pin          GPIO_PIN_5      /* Arduino D4 - boton            */
#define FLOW_BTN_GPIO_Port    GPIOB

/* ===================== Parametros de caracterizacion ========================
   YF-S201 tipico: ~450 pulsos/litro  ->  F(Hz) = 7.5 x Q(L/min).
   AJUSTA pulsos/litro con TU medicion (correr volumen conocido). */
#define FLOW_PULSES_PER_LITER 450U
#define FLOW_WINDOW_MS        1000U    /* ventana de medida (1 s) */
#define FLOW_BTN_DEBOUNCE_MS  200U

/* 0 = solo log por Vcom (caracterizacion). 1 = ademas TX Sigfox. */
#ifndef FLOW_TX_ENABLE
#define FLOW_TX_ENABLE        1
#endif

/* Totalizador: manda un uplink cada vez que se acumulan estos litros.
   El payload lleva el VOLUMEN TOTAL acumulado (no se reinicia). Asi, aunque
   se pierda un mensaje, el siguiente ya trae el total correcto.
   OJO duty-cycle Sigfox (~140 msg/dia): con 10 L/uplink solo puedes reportar
   ~1400 L/dia. Si el flujo continuo es alto, SUBE este umbral (ej. 100 L). */
#define FLOW_UPLINK_LITERS    10U

/* Diagnostico: 0 = el boton SOLO loguea (no transmite) para aislar si el
   reinicio es por el TX o por el pin. 1 = el boton manda uplink del total. */
#ifndef FLOW_BTN_TX
#define FLOW_BTN_TX           0
#endif

/* ===================== API publica ========================================= */
void Flow_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLOW_APP_H__ */
