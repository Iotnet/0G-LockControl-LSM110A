/**
  ******************************************************************************
  * @file    hall_door_app.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Monitoreo de APERTURA DE PUERTA con sensor Hall OMNIPOLAR (sin
  *          polaridad: AH1815, SL353, DRV5032 o similar, salida digital).
  *
  *          A diferencia de hall_app (sondeo periodico), este modulo usa la
  *          interrupcion EXTI de AMBOS FLANCOS configurada por CubeMX en el
  *          .ioc (PA1 = HALL_DOOR, pull-up, NVIC EXTI1) segun la regla de
  *          arquitectura: los perifericos de cada programa se declaran en su
  *          propio .ioc y CubeMX genera el codigo (gpio.c, stm32wlxx_it.c).
  *
  *          Logica: EXTI (cualquier flanco) -> timer one-shot de debounce ->
  *          tarea del sequencer lee el nivel ESTABLE -> si cambio respecto al
  *          ultimo TX, envia uplink Sigfox (apertura Y cierre), respetando
  *          una ventana minima entre uplinks y un tope diario (RC2 Mexico:
  *          140 uplinks/dia de contrato).
  *
  *          Payload de evento (6 bytes):
  *            byte0    = bit1: 0=ABIERTA / 1=CERRADA   bit0: 1=hora confiable
  *                       (0x00 abierta / 0x02 cerrada, +0x01 si el reloj fue
  *                       sincronizado por downlink)
  *            byte1..3 = hora del evento HH MM SS (RTC via SysTime)
  *            byte4..5 = contador de eventos confirmados (uint16 BE)
  *
  *          Reloj: siembra automatica con hora de compilacion al arrancar +
  *          sincronizacion por DOWNLINK Sigfox (peticion 0xF0 al boot, luego
  *          re-sync cada DOOR_TIME_RESYNC_DAYS). Respuesta del backend:
  *          8 bytes = epoch Unix UTC uint32 BE + 4 reservados.
  *          Ajuste manual opcional: AT$TIME=HH:MM:SS.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __HALL_DOOR_APP_H__
#define __HALL_DOOR_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"   /* HALL_DOOR_Pin / HALL_DOOR_GPIO_Port (generados del .ioc) */

/* ===================== Sensor ===============================================
   Hall omnipolar con salida open-drain (o push-pull activo bajo): iman
   presente => salida en BAJO. Pull-up interno activado en el .ioc; si usas
   el filtro RC externo (10k a 3V3 + 1nF a GND, recomendado en instalacion
   real) el pull-up interno solo actua de respaldo.
   El boton B2 de la Nucleo comparte PA1: presionado = "iman presente"
   (puerta cerrada) => sirve de SIMULADOR de puerta sin cablear nada. */
#define DOOR_ACTIVE_LOW        1     /* 1: iman presente = nivel BAJO         */

/* ===================== Reloj ================================================ */
/* 1: al arrancar, si el reloj no ha sido puesto en hora, se siembra AUTOMATICO
   con la hora de compilacion (__TIME__). Como se flashea justo despues de
   compilar, queda a segundos de la hora real sin teclear nada. Ajuste fino
   opcional: AT$TIME=HH:MM:SS. Ojo: tras un corte de alimentacion de dias, la
   siembra usa la hora del ULTIMO build (vieja) — re-flashea o usa AT$TIME. */
#define DOOR_CLOCK_SEED_BUILD_TIME  1

/* Base de epoch para siembra/ajuste SIN fecha (build time o AT$TIME):
   2000-01-01 00:00 UTC. Multiplo exacto de 86400 para que (Seconds % 86400)
   siga dando la hora del dia. Queda DEBAJO del umbral de "hora real", asi el
   sistema distingue: seed/AT (best-effort) vs downlink (epoch real, confiable). */
#define DOOR_CLOCK_EPOCH_BASE       946684800UL

/* Umbral de "hora real": todo epoch >= 2025-01-01 se considera sincronizado
   con fecha verdadera (solo lo produce el downlink del backend). */
#define DOOR_CLOCK_REAL_EPOCH_MIN   1735689600UL

/* ===================== Sincronizacion por downlink ========================== */
/* 1 = pide la hora al backend por downlink Sigfox: al arrancar (si el reloj
   no es confiable), reintento cada 24 h hasta lograrlo, y re-sync cada
   DOOR_TIME_RESYNC_DAYS. Maximo 1 downlink/dia (limite Sigfox: 4/dia).
   Respuesta esperada (8 bytes): [epoch UTC uint32 BE][4 reservados]. */
#define DOOR_TIME_SYNC_ENABLE       1
#define DOOR_TIME_RESYNC_DAYS       7U      /* re-sync semanal por deriva     */
#define DOOR_TIME_BOOT_SYNC_MS      8000U   /* espera tras boot para pedirla  */
#define DOOR_TIME_REQ_BYTE          0xF0U   /* byte de "peticion de hora"     */
#define DOOR_TZ_OFFSET_S            (-21600L) /* UTC-6 CDMX; el reloj guarda
                                                 epoch LOCAL para que el
                                                 payload lleve hora local     */

/* ===================== Cadencias / limites ================================== */
#define DOOR_DEBOUNCE_MS       200U  /* nivel estable tras el ultimo flanco   */
#define DOOR_TX_MIN_GAP_MS     10000U/* ventana minima entre uplinks (colapsa
                                        rafagas; el estado FINAL siempre se
                                        reporta al expirar la ventana)        */
#define DOOR_MAX_TX_PER_DAY    100U  /* tope duro de uplinks/24h (0 = sin tope)*/
#define DOOR_TX_STARTUP_STATE  0     /* 1: envia el estado inicial al arrancar */

/* ===================== API publica ========================================= */

/**
  * @brief  Registra la tarea del sequencer y los timers. El pin lo configura
  *         MX_GPIO_Init() (CubeMX). Llamar desde Sigfox_Init() bajo
  *         #if USE_DOOR_APP.
  */
void DoorHall_Init(void);

/**
  * @brief  Notificacion EXTI (llamar desde HAL_GPIO_EXTI_Callback). Ignora
  *         pines ajenos; para HALL_DOOR re-arma el timer de debounce.
  */
void DoorHall_HandleEXTI(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif

#endif /* __HALL_DOOR_APP_H__ */
