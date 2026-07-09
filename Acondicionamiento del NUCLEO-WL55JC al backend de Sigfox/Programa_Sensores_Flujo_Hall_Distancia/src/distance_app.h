/**
  ******************************************************************************
  * @file    distance_app.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Aplicacion de sensores de distancia (2x VL53L0X/VL53L1X detras de
  *          un multiplexor I2C PCA9548A) para el firmware Sigfox_AT_Slave.
  *
  *          Mismo patron que buttons_app: un <Modulo>_Init() invocado desde
  *          Sigfox_Init() y una tarea registrada en el sequencer de ST. A
  *          diferencia de los botones (evento externo), la lectura es
  *          PERIODICA: un UTIL_TIMER dispara la tarea, que lee ambos sensores
  *          por el mux y envia un uplink Sigfox con las dos distancias.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __DISTANCE_APP_H__
#define __DISTANCE_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ===================== Parametros configurables ============================ */

/* Direccion I2C del PCA9548A (A2A1A0 = 000 -> 0x70). Formato HAL de 8 bits. */
#define PCA9548A_ADDR_7B        0x70U
#define PCA9548A_ADDR           (PCA9548A_ADDR_7B << 1)

/* Canales del mux donde estan cableados los dos sensores. */
#define DIST_SENSOR_A_CH        0U
#define DIST_SENSOR_B_CH        1U

/* Direccion I2C por defecto de un VL53L0X/L1X (0x29). Formato HAL de 8 bits.
   Gracias al mux NO hay que reasignarla: cada sensor vive en su canal. */
#define VL53_I2C_ADDR           (0x29U << 1)

/* ---- Modo de payload -------------------------------------------------------
   DIST_MODE_MM    : 4 bytes = dA,dB en mm (envio periodico cada DIST_PERIOD_MS)
   DIST_MODE_FLAGS : 1 byte  = bits de presencia con histeresis (envio por evento)
   DIST_MODE_BOTH  : 5 bytes = 1 byte de flags + dA,dB en mm                    */
#define DIST_MODE_MM            0
#define DIST_MODE_FLAGS         1
#define DIST_MODE_BOTH          2

#ifndef DIST_PAYLOAD_MODE
#define DIST_PAYLOAD_MODE       DIST_MODE_FLAGS
#endif

/* ---- Umbrales de PRESENCIA con HISTERESIS (solo modos FLAGS / BOTH) ---------
   Banda muerta entre NEAR y FAR: el bit NO cambia mientras la medida quede
   dentro de [NEAR, FAR]. Asi el estado no titila cuando el objeto queda justo
   en el limite. Debe cumplirse NEAR < FAR.                                     */
/* PUERTA: cerca = cerrada, lejos = abierta.
   Tu sensor marca ~21-40 mm con la puerta CERRADA y mas con la puerta ABIERTA.
   CERRADA cuando la distancia < NEAR ; ABIERTA cuando la distancia > FAR.
   La banda [NEAR, FAR] es histeresis (margen sobre el limite de ~40 mm) para
   que el estado no rebote. AJUSTA segun lo que veas en el log (el sensor trae
   un offset de fabrica). Debe cumplirse NEAR < FAR. */
#define DIST_NEAR_MM            45U    /* < 45 mm  -> PUERTA CERRADA (cubre 21-40) */
#define DIST_FAR_MM             60U    /* > 60 mm  -> PUERTA ABIERTA */

/* ---- Cadencias -------------------------------------------------------------
   En modo presencia se MUESTREA rapido (barato, sin radio) y solo se TRANSMITE
   cuando cambia el estado o al vencer el heartbeat. En modo MM se muestrea y
   envia cada DIST_PERIOD_MS. Todo se cuenta en ciclos del timer, que corre
   sobre RTC y por tanto sigue avanzando en STOP2 (a diferencia de HAL_GetTick).*/
#define DIST_PERIOD_MS          (15U * 60U * 1000U)   /* modo MM: muestreo+TX   */
#define DIST_SAMPLE_MS          (5U * 1000U)          /* lee cada 5 s (ahorro) */
#define DIST_MIN_TX_INTERVAL_MS (5U * 1000U)          /* guard = 1 muestra: envia cada evento */
#define DIST_HEARTBEAT_MS       0U                    /* 0 = SOLO por evento, sin periodicidad */

/* Periodo real del timer segun el modo elegido. */
#if (DIST_PAYLOAD_MODE == DIST_MODE_MM)
#define DIST_TIMER_MS           DIST_PERIOD_MS
#else
#define DIST_TIMER_MS           DIST_SAMPLE_MS
#endif

/* ---- Candado anti-abuso de banda ------------------------------------------
   Tope DURO de uplinks por ventana de 24 h. Sigfox RC2/RC4 (Mexico) permite
   ~140/dia; dejamos margen. Aunque la deteccion "flapee" o falle un sensor,
   NUNCA se rebasa este numero. La histeresis y el guard reducen el trafico;
   este tope lo garantiza. 0 = sin limite. */
#ifndef DIST_MAX_TX_PER_DAY
#define DIST_MAX_TX_PER_DAY     100U
#endif

/* Replicas de radio por uplink (1 = solo TX1; 3 = TX1/TX2/TX3). */
#define DIST_TX_REPLICAS        1U

/* 1 = envia por Sigfox; 0 = solo lee y loguea por Vcom (util en bring-up). */
#ifndef DIST_TX_ENABLE
#define DIST_TX_ENABLE          1
#endif

/* 1 = usa el ULD oficial de ST (VL53L1X_api.c/.h presentes en Sigfox/App).
   0 = modo bring-up: solo verifica el bus leyendo el Model ID del sensor
       por cada canal del mux (no requiere el ULD, compila y linka ya). */
#ifndef DIST_HAVE_VL53_ULD
#define DIST_HAVE_VL53_ULD      0
#endif

/* ===================== API publica ========================================= */

/**
  * @brief  Inicializa I2C1, los dos sensores VL53 tras el PCA9548A, registra
  *         la tarea en el sequencer y arranca el timer periodico.
  *         Llamar desde Sigfox_Init() (bajo #if USE_DISTANCE_APP).
  */
void Distance_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DISTANCE_APP_H__ */
