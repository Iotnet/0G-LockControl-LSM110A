/**
  ******************************************************************************
  * @file    lis2dw12.h
  * @brief   Driver minimalista para acelerometro LIS2DW12 (I2C) en 0G LockControl
  * @author  Yahir Flores - 0G IoT Solutions
  *
  * @note    Pinout (LSM110A):
  *          - I2C1_SCL  -> PA9
  *          - I2C1_SDA  -> PA10
  *          - INT1      -> PA0 (EXTI0, wake-up)
  *
  * @note    Para el Nucleo-WL55JC ajustar el handle I2C y pin de INT1 segun
  *          el cableado real (typicamente PB8/PB9 si se usa Arduino header).
  *
  ******************************************************************************
  */

#ifndef LIS2DW12_H
#define LIS2DW12_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32wlxx_hal.h"

/* I2C 7-bit address (SA0 = GND en LSM110A) */
#define LIS2DW12_I2C_ADDR_7BIT   0x18
#define LIS2DW12_I2C_ADDR_8BIT   (LIS2DW12_I2C_ADDR_7BIT << 1)

/* Valor esperado de WHO_AM_I */
#define LIS2DW12_WHO_AM_I_VAL    0x44

/* Registros usados */
#define LIS2DW12_REG_WHO_AM_I        0x0F
#define LIS2DW12_REG_CTRL1           0x20
#define LIS2DW12_REG_CTRL2           0x21
#define LIS2DW12_REG_CTRL3           0x22
#define LIS2DW12_REG_CTRL4_INT1_PAD  0x23
#define LIS2DW12_REG_CTRL5_INT2_PAD  0x24
#define LIS2DW12_REG_CTRL6           0x25
#define LIS2DW12_REG_CTRL7           0x3F
#define LIS2DW12_REG_WAKE_UP_THS     0x34
#define LIS2DW12_REG_WAKE_UP_DUR     0x35
#define LIS2DW12_REG_WAKE_UP_SRC     0x38

/* CTRL1: ODR=12.5Hz (modo low-power), low-power mode 1, lp_mode=00 */
#define LIS2DW12_CTRL1_ODR_12_5HZ_LP   0x20   /* ODR[3:0]=0010, MODE[1:0]=00, LP_MODE[1:0]=00 */

/* CTRL6: full-scale ±2g (FS[1:0]=00), low-pass filter ODR/2 */
#define LIS2DW12_CTRL6_FS_2G           0x00

/* CTRL4_INT1_PAD: rutea wake-up a INT1 (bit5 = INT1_WU) */
#define LIS2DW12_CTRL4_INT1_WU         0x20

/* CTRL7: habilita interrupcion (bit5 = INTERRUPTS_ENABLE) */
#define LIS2DW12_CTRL7_INT_EN          0x20

/* WAKE_UP_SRC bits (lectura tras evento) */
#define LIS2DW12_WU_SRC_WU_IA          0x40   /* wake-up event detected */
#define LIS2DW12_WU_SRC_X              0x04
#define LIS2DW12_WU_SRC_Y              0x02
#define LIS2DW12_WU_SRC_Z              0x01

/**
 * @brief Status del driver
 */
typedef enum {
    LIS2DW12_OK = 0,
    LIS2DW12_ERR_I2C,
    LIS2DW12_ERR_WHO_AM_I,
    LIS2DW12_ERR_PARAM,
} lis2dw12_status_t;

/**
 * @brief Estructura del dispositivo. Por inyeccion el HAL handle de I2C.
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;     /* p.ej. &hi2c1 */
    uint16_t i2c_timeout_ms;     /* default 100 */
} lis2dw12_t;

/* ---------- API ---------- */

/**
 * @brief Inicializa la estructura (no toca HW todavia).
 */
void lis2dw12_init_handle(lis2dw12_t *dev, I2C_HandleTypeDef *hi2c);

/**
 * @brief Lee WHO_AM_I. Debe devolver 0x44.
 * @retval LIS2DW12_OK si who_am_i == 0x44, codigo de error si no.
 */
lis2dw12_status_t lis2dw12_who_am_i(lis2dw12_t *dev, uint8_t *out);

/**
 * @brief Configura el modo wake-up: ODR=12.5Hz LP, threshold en mg, INT1 a PA0.
 *
 * @param threshold_mg  Umbral en miligs (recomendado 200). El LIS2DW12 usa 6 bits
 *                      escalados a FS, asi que la resolucion real es FS/64.
 *                      Para FS=2g: 1 LSB = 31.25mg → 200mg ≈ 6 LSB.
 *
 * @retval LIS2DW12_OK si todo se escribio correctamente.
 */
lis2dw12_status_t lis2dw12_config_wakeup(lis2dw12_t *dev, uint16_t threshold_mg);

/**
 * @brief Lee el registro WAKE_UP_SRC (limpia el flag al leer).
 *
 * @param[out] src  Valor crudo del registro. Usar masks LIS2DW12_WU_SRC_*
 */
lis2dw12_status_t lis2dw12_read_wake_source(lis2dw12_t *dev, uint8_t *src);

/* Helpers raw I2C (expuestos para debug) */
lis2dw12_status_t lis2dw12_read_reg(lis2dw12_t *dev, uint8_t reg, uint8_t *val);
lis2dw12_status_t lis2dw12_write_reg(lis2dw12_t *dev, uint8_t reg, uint8_t val);

#endif /* LIS2DW12_H */
