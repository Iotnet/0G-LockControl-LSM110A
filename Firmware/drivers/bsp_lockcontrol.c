/**
  ******************************************************************************
  * @file    bsp_lockcontrol.c
  * @brief   Implementacion del BSP: I2C1 + GPIO EXTI0/1.
  * @author  Yahir Flores - 0G IoT Solutions
  ******************************************************************************
  */

#include "bsp_lockcontrol.h"

/* Lo provee la aplicacion (main.c del SDK); se declara aqui para no acoplar
   el BSP a main.h */
extern void Error_Handler(void);

/* Handle global */
I2C_HandleTypeDef hi2c1;

/* ----------------------------- I2C1 ----------------------------- */

static void MX_I2C1_Init(void)
{
    hi2c1.Instance              = I2C1;
    /* 100 kHz Standard Mode @ 16 MHz PCLK1 (HSI). Valor oficial STM32WL. */
    hi2c1.Init.Timing           = 0x10707DBC;
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
}

/* MSP del I2C1: clocks + pines + NVIC */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C1) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* I2C1 clock source = PCLK1 */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection   = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }

    /* GPIO clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9 = SCL, PA10 = SDA */
    GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;     /* pull-up externos 4.7k */
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral clock */
    __HAL_RCC_I2C1_CLK_ENABLE();
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C1) return;
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
}

/* ----------------------------- GPIO EXTI ----------------------------- */

static void MX_GPIO_LockControl_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA0 -> ACCEL_INT1: input EXTI rising, pull-down */
    GPIO_InitStruct.Pin  = ACCEL_INT1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(ACCEL_INT1_GPIO_PORT, &GPIO_InitStruct);

    /* PA1 -> REED: input EXTI both edges, pull-up */
    GPIO_InitStruct.Pin  = REED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(REED_GPIO_PORT, &GPIO_InitStruct);

    /* NVIC */
    HAL_NVIC_SetPriority(ACCEL_INT1_EXTI_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ACCEL_INT1_EXTI_IRQn);

    HAL_NVIC_SetPriority(REED_EXTI_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(REED_EXTI_IRQn);
}

/* ----------------------------- Watchdog ----------------------------- */

#if LOCKCONTROL_IWDG_ENABLE

static IWDG_HandleTypeDef hiwdg;

static void MX_IWDG_Init(void)
{
    /* LSI ~32 kHz / prescaler 256 = 125 Hz de conteo.
       Reload = timeout_ms * 125 / 1000 (16 s -> 2000; max 4095 ~32.7 s). */
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload    = (LOCKCONTROL_IWDG_TIMEOUT_MS * 125U) / 1000U;
    hiwdg.Init.Window    = IWDG_WINDOW_DISABLE;

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}

void BSP_LockControl_Watchdog_Refresh(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

#else /* !LOCKCONTROL_IWDG_ENABLE */

void BSP_LockControl_Watchdog_Refresh(void)
{
    /* watchdog deshabilitado: ver nota en bsp_lockcontrol.h */
}

#endif /* LOCKCONTROL_IWDG_ENABLE */

/* ----------------------------- Entry point ----------------------------- */

void BSP_LockControl_Init(void)
{
    MX_I2C1_Init();
    MX_GPIO_LockControl_Init();
#if LOCKCONTROL_IWDG_ENABLE
    MX_IWDG_Init();
#endif
}

/* ----------------------------- IRQ Handlers ----------------------------- */

/* EXTI line 0 -> PA0 (ACCEL_INT1) */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(ACCEL_INT1_PIN);
}

/* EXTI line 1 -> PA1 (REED) */
void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(REED_PIN);
}
