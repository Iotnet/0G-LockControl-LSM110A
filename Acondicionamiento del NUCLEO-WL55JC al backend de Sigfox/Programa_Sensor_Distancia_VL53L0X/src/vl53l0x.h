/**
  ******************************************************************************
  * @file    vl53l0x.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Driver compacto y autocontenido para el sensor de distancia
  *          VL53L0X (registros de 8 bits) sobre el HAL I2C de STM32WL.
  *          Basado en la secuencia de init publica de ST/Pololu (MIT).
  *          Suficiente para PRESENCIA (medicion continua, sin calibracion fina).
  ******************************************************************************
  * Empresa: 0G IoT Solutions (previamente WND Mexico) - https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __VL53L0X_H__
#define __VL53L0X_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
  * @brief  Inicializa el VL53L0X y arranca medicion continua.
  * @retval 1 = OK, 0 = fallo (no responde / ID incorrecto).
  */
uint8_t VL53L0X_Init(void);

/**
  * @brief  Lee la ultima distancia medida (mm) en modo continuo.
  * @retval distancia en mm; ~8190 = fuera de rango; 0xFFFF = timeout/error.
  */
uint16_t VL53L0X_ReadRangeContinuousMillimeters(void);

#ifdef __cplusplus
}
#endif

#endif /* __VL53L0X_H__ */
