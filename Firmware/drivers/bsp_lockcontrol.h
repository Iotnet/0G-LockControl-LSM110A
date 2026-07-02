/**
  ******************************************************************************
  * @file    bsp_lockcontrol.h
  * @brief   Board Support Package del 0G LockControl LSM110A.
  *          Inicializa I2C1 (PA9/PA10) y GPIOs EXTI (PA0/PA1) que el SDK
  *          base de SJI no configura por su cuenta.
  * @author  Yahir Flores - 0G IoT Solutions
  *
  * @note    Llamar BSP_LockControl_Init() una vez al boot, despues de
  *          SystemClock_Config() y antes de tocar los drivers.
  ******************************************************************************
  */

#ifndef BSP_LOCKCONTROL_H
#define BSP_LOCKCONTROL_H

#include "stm32wlxx_hal.h"

/* Handle I2C expuesto para los drivers */
extern I2C_HandleTypeDef hi2c1;

/* Pines del proyecto */
#define ACCEL_INT1_GPIO_PORT   GPIOA
#define ACCEL_INT1_PIN         GPIO_PIN_0
#define ACCEL_INT1_EXTI_IRQn   EXTI0_IRQn

#define REED_GPIO_PORT         GPIOA
#define REED_PIN               GPIO_PIN_1
#define REED_EXTI_IRQn         EXTI1_IRQn

/* ----------------------------- Watchdog (Y5) -----------------------------
 * IMPORTANTE: el IWDG corre desde LSI y NO se congela en Stop2 (default de
 * option bytes). El build actual del SDK define LPUART => LOW_POWER_DISABLE=0
 * => el MCU duerme en Stop2 en idle, donde nadie refresca => reset ciclico.
 * Por eso el default es 0. Habilitar SOLO en builds de debug/bench con
 * LOW_POWER_DISABLE=1, o en M4 tras programar el option byte IWDG_STOP=frozen.
 */
#ifndef LOCKCONTROL_IWDG_ENABLE
#define LOCKCONTROL_IWDG_ENABLE      0
#endif

/* Timeout > peor caso de TX Sigfox RC2 bloqueante (3 frames a 600 bps ~7-9 s).
 * Con 4 s (valor original del handoff) el watchdog cortaria cada transmision. */
#define LOCKCONTROL_IWDG_TIMEOUT_MS  16000U

/**
 * @brief Inicializa I2C1 + GPIOs EXTI0/1 (+ IWDG si esta habilitado).
 *        Llamar una vez al boot.
 */
void BSP_LockControl_Init(void);

/**
 * @brief Refresca el IWDG. No-op si LOCKCONTROL_IWDG_ENABLE=0.
 *        Llamar en cada iteracion del main loop (MX_Sigfox_Process).
 */
void BSP_LockControl_Watchdog_Refresh(void);

#endif /* BSP_LOCKCONTROL_H */
