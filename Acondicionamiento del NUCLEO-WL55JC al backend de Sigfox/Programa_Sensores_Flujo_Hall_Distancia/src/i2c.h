/**
  ******************************************************************************
  * @file    i2c.h
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Cabecera para I2C1 (bus del multiplexor PCA9548A + sensores VL53).
  *
  *          Este periferico NO venia en el proyecto Sigfox_AT_Slave original.
  *          Se anade a mano siguiendo el patron CubeMX (MX_I2C1_Init +
  *          HAL_I2C_MspInit). Se inicializa de forma perezosa desde
  *          Distance_Init(), por lo que si USE_DISTANCE_APP==0 el I2C nunca
  *          se enciende (cero consumo extra en la build de botones).
  *
  *          Pines por defecto (ambos libres en la NUCLEO-WL55JC1):
  *            SCL = PB8 (AF4)   SDA = PB7 (AF4)
  *          Ajusta I2C1_SCL_* / I2C1_SDA_* si tu cableado difiere.
  *
  *          NOTA CubeMX: si mas adelante habilitas I2C1 desde el .ioc,
  *          CubeMX generara su propio i2c.c/i2c.h y MX_I2C1_Init. En ese
  *          caso ELIMINA este par de archivos para evitar doble definicion.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* === Mapeo de pines ===
   I2C2 en los pines Arduino ROTULADOS SCL y SDA (D15/D14) = PA12 y PA11.
   Estos pines NO estan cargados por el ST-LINK (a diferencia de D0/PB7, que
   en open-drain solo llegaba a ~1.7V y no servia como SDA).
     SCL = PA12 (Arduino "SCL" / D15)
     SDA = PA11 (Arduino "SDA" / D14)
   NOTA: conservamos los nombres I2C1 y hi2c1 en el resto del codigo, pero el
   periferico real es I2C2. Pull-up 4.7k a 3.3V en CADA linea. */
#define I2C1_SCL_Pin        GPIO_PIN_12
#define I2C1_SCL_GPIO_Port  GPIOA
#define I2C1_SDA_Pin        GPIO_PIN_11
#define I2C1_SDA_GPIO_Port  GPIOA
#define I2C1_GPIO_AF        GPIO_AF4_I2C2

/* Timing para 100 kHz (Standard mode) con PCLK1 = 32 MHz (MSI range 10,
   como configura SystemClock_Config en este proyecto).
   Si cambias el reloj, recalcula con la herramienta de timing de CubeMX. */
#define I2C1_TIMING_100KHZ  0x70421313U

extern I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */
