/**
  ******************************************************************************
  * @file    app_features.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Selector de aplicaciones de usuario (feature flags) para el
  *          firmware Sigfox_AT_Slave sobre NUCLEO-WL55JC1.
  *
  *          Permite activar/desactivar modulos de usuario en TIEMPO DE
  *          COMPILACION sin borrar codigo. Cada modulo (buttons_app,
  *          distance_app, ...) se envuelve con su flag y su llamada a
  *          <Modulo>_Init() en Sigfox_Init() queda condicionada.
  *
  *          Los flags usan #ifndef para que puedan sobre-escribirse desde
  *          los "Preprocessor Defined symbols" de una Build Configuration
  *          de STM32CubeIDE (p.ej. -D USE_BUTTONS_APP=0). Asi puedes tener:
  *
  *            - Config "Debug_Buttons"  : USE_BUTTONS_APP=1  USE_DISTANCE_APP=0
  *            - Config "Debug_Distance" : USE_BUTTONS_APP=0  USE_DISTANCE_APP=1
  *
  *          y cambiar de aplicacion solo cambiando de configuracion, sin
  *          tocar el codigo. Si no defines nada por -D, mandan los valores
  *          por defecto de este archivo.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __APP_FEATURES_H__
#define __APP_FEATURES_H__

#ifdef __cplusplus
extern "C" {
#endif

/* === Aplicacion de push-buttons B1/B2/B3 (buttons_app.c) ===
   1 = activa (cada push dispara un TX Sigfox)
   0 = omitida (el .c compila vacio, nunca se inicializa ni agenda tarea) */
#ifndef USE_BUTTONS_APP
#define USE_BUTTONS_APP    0
#endif

/* === Aplicacion de sensores de distancia VL53L0X/L1X + PCA9548A (distance_app.c) ===
   1 = activa (lectura periodica de 2 sensores por el mux I2C y TX Sigfox)
   0 = omitida */
#ifndef USE_DISTANCE_APP
#define USE_DISTANCE_APP   0   /* apagada mientras probamos el Hall (VL53 desconectado) */
#endif

/* === Aplicacion de sensor Hall MAGNETICO + boton externo (hall_app.c) ===
   Version puerta (abierto/cerrado). Se conserva pero apagada. */
#ifndef USE_HALL_APP
#define USE_HALL_APP       0
#endif

/* === Aplicacion de sensor de FLUJO de efecto Hall + boton (flow_app.c) ===
   Cuenta pulsos por EXTI -> frecuencia (Hz) -> L/min + volumen. Para
   caracterizacion del sensor de flujo. 1 = activa, 0 = omitida. */
#ifndef USE_FLOW_APP
#define USE_FLOW_APP       1
#endif

/* Chequeo de coherencia: al menos deja constancia si ambas quedan activas.
   No es un error (comparten el mismo TX de radio de forma cooperativa via
   sequencer), pero recuerda respetar el duty-cycle Sigfox si las combinas. */
#if (USE_BUTTONS_APP == 0) && (USE_DISTANCE_APP == 0)
#warning "app_features.h: no hay ninguna aplicacion de usuario activa."
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APP_FEATURES_H__ */
