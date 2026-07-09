/**
  ******************************************************************************
  * @file    i2c.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Inicializacion de I2C1 para el multiplexor PCA9548A y los
  *          sensores de distancia VL53L0X/VL53L1X.
  *
  *          Uso en modo BLOQUEANTE (polling) -> no requiere NVIC ni DMA ni
  *          handlers en stm32wlxx_it.c. Las lecturas del VL53 son cortas y
  *          se ejecutan dentro de la tarea del sequencer (contexto de hilo),
  *          por lo que bloquear unos ms es seguro y compatible con LPM.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

I2C_HandleTypeDef hi2c1;

/**
  * @brief I2C1 Initialization Function (100 kHz, 7-bit addressing).
  */
void MX_I2C1_Init(void)
{
  hi2c1.Instance             = I2C2;   /* pines Arduino SDA/SCL = I2C2 (PA11/PA12) */
  hi2c1.Init.Timing          = I2C1_TIMING_100KHZ;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2     = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Filtro analogico ON, filtro digital OFF (valores por defecto sanos). */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C MSP Initialization. Sobre-escribe la funcion weak del HAL.
  *        Habilita relojes y configura los pines SCL/SDA en Open-Drain + AF.
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *i2cHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (i2cHandle->Instance == I2C2)
  {
    /* SCL (PA12) y SDA (PA11) estan en GPIOA. */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* SCL y SDA: alterno abierto (Open-Drain), pull-up interno de apoyo.
       IMPORTANTE: manten pull-ups externos (2.2k-4.7k a 3V3) en el bus;
       los internos por si solos no bastan para un bus con mux + 2 sensores. */
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = I2C1_GPIO_AF;

    GPIO_InitStruct.Pin = I2C1_SCL_Pin;
    HAL_GPIO_Init(I2C1_SCL_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = I2C1_SDA_Pin;
    HAL_GPIO_Init(I2C1_SDA_GPIO_Port, &GPIO_InitStruct);

    __HAL_RCC_I2C2_CLK_ENABLE();
  }
}

/**
  * @brief I2C MSP De-Initialization.
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *i2cHandle)
{
  if (i2cHandle->Instance == I2C2)
  {
    __HAL_RCC_I2C2_CLK_DISABLE();
    HAL_GPIO_DeInit(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin);
    HAL_GPIO_DeInit(I2C1_SDA_GPIO_Port, I2C1_SDA_Pin);
  }
}
