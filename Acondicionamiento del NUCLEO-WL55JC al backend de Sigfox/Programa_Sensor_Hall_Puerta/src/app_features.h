/**
  ******************************************************************************
  * @file    app_features.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Selector de aplicacion de usuario (feature flag) para el programa
  *          Programa_Sensor_Hall_Puerta sobre NUCLEO-WL55JC1.
  *
  *          Regla de arquitectura: 1 programa = 1 proyecto = 1 .ioc = 1 app.
  *          Este proyecto contiene UNICAMENTE hall_door_app (sensor Hall
  *          omnipolar de puerta). El flag usa #ifndef para poder apagarse
  *          desde los "Preprocessor Defined symbols" de una Build
  *          Configuration de STM32CubeIDE (p.ej. -D USE_DOOR_APP=0 para un
  *          build AT-only de diagnostico), sin tocar el codigo.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 2.0 (limpieza: se retiraron los modulos heredados del proyecto
  *          compartido; viven en sus propios programas/proyectos)
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __APP_FEATURES_H__
#define __APP_FEATURES_H__

#ifdef __cplusplus
extern "C" {
#endif

/* === Aplicacion de PUERTA con sensor Hall OMNIPOLAR (hall_door_app.c) ===
   *** LA APLICACION DE ESTE PROGRAMA ***
   Sensor sin polaridad (AH1815/SL353/DRV5032) en PA1 = HALL_DOOR, EXTI1
   ambos flancos configurado en el .ioc. Uplink al ABRIR y al CERRAR con
   debounce, ventana minima entre TX y tope diario. B2 = simulador.
   1 = activa, 0 = omitida (queda solo la interfaz AT). */
#ifndef USE_DOOR_APP
#define USE_DOOR_APP       1
#endif

#if (USE_DOOR_APP == 0)
#warning "app_features.h: USE_DOOR_APP=0 -> build AT-only, sin app de puerta."
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APP_FEATURES_H__ */
