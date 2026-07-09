/**
  ******************************************************************************
  * @file    vl53l1_platform.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Capa de plataforma (glue) para el ULD de ST VL53L1X sobre STM32WL.
  *
  *          El ULD (VL53L1X_api.c / VL53L1X_api.h, paquete STSW-IMG009 de ST)
  *          NO se incluye aqui por licencia: descargalo de st.com y copia
  *          esos 2 archivos a Sigfox/App/. Este shim implementa las funciones
  *          de acceso al bus que el ULD espera, usando el HAL I2C de STM32.
  *
  *          El parametro 'dev' es la direccion I2C de 8 bits del sensor
  *          (0x52 = 0x29<<1 por defecto). La seleccion de canal del PCA9548A
  *          se hace FUERA de este shim (en distance_app.c) antes de llamar
  *          al ULD, para mantener el shim generico.
  ******************************************************************************
  * Empresa: 0G IoT Solutions (previamente WND Mexico) - https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __VL53L1_PLATFORM_H__
#define __VL53L1_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count);
int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count);
int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data);
int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data);
int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data);
int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data);
int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data);
int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data);
int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* __VL53L1_PLATFORM_H__ */
