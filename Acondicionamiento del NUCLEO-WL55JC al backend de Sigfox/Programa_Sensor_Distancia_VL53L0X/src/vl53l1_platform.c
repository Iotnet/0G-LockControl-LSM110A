/**
  ******************************************************************************
  * @file    vl53l1_platform.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Implementacion del shim de plataforma del ULD VL53L1X sobre el
  *          HAL I2C de STM32WL (modo bloqueante). Registros de 16 bits.
  ******************************************************************************
  * Empresa: 0G IoT Solutions (previamente WND Mexico) - https://0giotsolutions.com/
  ******************************************************************************
  */

#include "vl53l1_platform.h"
#include "i2c.h"          /* hi2c1 */

#define VL53_I2C_TIMEOUT   100U   /* ms por transaccion */

/* HAL usa MemAddSize de 16 bits porque el VL53L1X direcciona registros de 16b. */

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count)
{
  return (int8_t)HAL_I2C_Mem_Write(&hi2c1, dev, index, I2C_MEMADD_SIZE_16BIT,
                                   pdata, (uint16_t)count, VL53_I2C_TIMEOUT);
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count)
{
  return (int8_t)HAL_I2C_Mem_Read(&hi2c1, dev, index, I2C_MEMADD_SIZE_16BIT,
                                  pdata, (uint16_t)count, VL53_I2C_TIMEOUT);
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data)
{
  return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data)
{
  uint8_t buf[2];
  buf[0] = (uint8_t)(data >> 8);
  buf[1] = (uint8_t)(data & 0xFF);
  return VL53L1_WriteMulti(dev, index, buf, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data)
{
  uint8_t buf[4];
  buf[0] = (uint8_t)(data >> 24);
  buf[1] = (uint8_t)(data >> 16);
  buf[2] = (uint8_t)(data >> 8);
  buf[3] = (uint8_t)(data & 0xFF);
  return VL53L1_WriteMulti(dev, index, buf, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data)
{
  return VL53L1_ReadMulti(dev, index, data, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data)
{
  uint8_t buf[2];
  int8_t st = VL53L1_ReadMulti(dev, index, buf, 2);
  *data = ((uint16_t)buf[0] << 8) | buf[1];
  return st;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data)
{
  uint8_t buf[4];
  int8_t st = VL53L1_ReadMulti(dev, index, buf, 4);
  *data = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
          ((uint32_t)buf[2] << 8)  | buf[3];
  return st;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms)
{
  (void)dev;
  HAL_Delay((uint32_t)wait_ms);
  return 0;
}
