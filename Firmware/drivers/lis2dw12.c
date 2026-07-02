/**
  ******************************************************************************
  * @file    lis2dw12.c
  * @brief   Implementacion del driver LIS2DW12
  * @author  Yahir Flores - 0G IoT Solutions
  ******************************************************************************
  */

#include "lis2dw12.h"

/* Resolucion del threshold para FS=2g: 1 LSB = 2g/64 = 31.25 mg ≈ 31 mg */
#define LIS2DW12_THS_LSB_MG_FS2G   31

void lis2dw12_init_handle(lis2dw12_t *dev, I2C_HandleTypeDef *hi2c)
{
    if (dev == NULL) return;
    dev->hi2c = hi2c;
    dev->i2c_timeout_ms = 100;
    dev->event_pending = false;
}

lis2dw12_status_t lis2dw12_read_reg(lis2dw12_t *dev, uint8_t reg, uint8_t *val)
{
    if (dev == NULL || dev->hi2c == NULL || val == NULL)
        return LIS2DW12_ERR_PARAM;

    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(dev->hi2c,
                                            LIS2DW12_I2C_ADDR_8BIT,
                                            reg,
                                            I2C_MEMADD_SIZE_8BIT,
                                            val,
                                            1,
                                            dev->i2c_timeout_ms);
    return (st == HAL_OK) ? LIS2DW12_OK : LIS2DW12_ERR_I2C;
}

lis2dw12_status_t lis2dw12_write_reg(lis2dw12_t *dev, uint8_t reg, uint8_t val)
{
    if (dev == NULL || dev->hi2c == NULL)
        return LIS2DW12_ERR_PARAM;

    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(dev->hi2c,
                                             LIS2DW12_I2C_ADDR_8BIT,
                                             reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             &val,
                                             1,
                                             dev->i2c_timeout_ms);
    return (st == HAL_OK) ? LIS2DW12_OK : LIS2DW12_ERR_I2C;
}

lis2dw12_status_t lis2dw12_who_am_i(lis2dw12_t *dev, uint8_t *out)
{
    uint8_t v = 0;
    lis2dw12_status_t s = lis2dw12_read_reg(dev, LIS2DW12_REG_WHO_AM_I, &v);
    if (s != LIS2DW12_OK) return s;
    if (out != NULL) *out = v;
    return (v == LIS2DW12_WHO_AM_I_VAL) ? LIS2DW12_OK : LIS2DW12_ERR_WHO_AM_I;
}

lis2dw12_status_t lis2dw12_config_wakeup(lis2dw12_t *dev, uint16_t threshold_mg)
{
    lis2dw12_status_t s;

    /* 1. Verificar WHO_AM_I antes de configurar */
    s = lis2dw12_who_am_i(dev, NULL);
    if (s != LIS2DW12_OK) return s;

    /* 2. CTRL1: ODR=12.5Hz, Low-Power Mode 1 */
    s = lis2dw12_write_reg(dev, LIS2DW12_REG_CTRL1, LIS2DW12_CTRL1_ODR_12_5HZ_LP);
    if (s != LIS2DW12_OK) return s;

    /* 3. CTRL6: full-scale ±2g */
    s = lis2dw12_write_reg(dev, LIS2DW12_REG_CTRL6, LIS2DW12_CTRL6_FS_2G);
    if (s != LIS2DW12_OK) return s;

    /* 4. WAKE_UP_THS: 6 bits de umbral. Para FS=2g, 1 LSB ≈ 31 mg. */
    uint8_t ths = (uint8_t)(threshold_mg / LIS2DW12_THS_LSB_MG_FS2G);
    if (ths > 0x3F) ths = 0x3F;            /* clamp a 6 bits */
    s = lis2dw12_write_reg(dev, LIS2DW12_REG_WAKE_UP_THS, ths);
    if (s != LIS2DW12_OK) return s;

    /* 5. WAKE_UP_DUR: 0 = pulso instantaneo. Ajustar si se quieren falsos positivos menos */
    s = lis2dw12_write_reg(dev, LIS2DW12_REG_WAKE_UP_DUR, 0x00);
    if (s != LIS2DW12_OK) return s;

    /* 6. CTRL4_INT1_PAD: rutea wake-up a INT1 */
    s = lis2dw12_write_reg(dev, LIS2DW12_REG_CTRL4_INT1_PAD, LIS2DW12_CTRL4_INT1_WU);
    if (s != LIS2DW12_OK) return s;

    /* 7. CTRL7: habilita interrupciones */
    s = lis2dw12_write_reg(dev, LIS2DW12_REG_CTRL7, LIS2DW12_CTRL7_INT_EN);
    return s;
}

lis2dw12_status_t lis2dw12_read_wake_source(lis2dw12_t *dev, uint8_t *src)
{
    return lis2dw12_read_reg(dev, LIS2DW12_REG_WAKE_UP_SRC, src);
}

/* ---------- ISR / app loop ---------- */

void lis2dw12_on_interrupt(lis2dw12_t *dev)
{
    if (dev == NULL) return;
    /* ISR context: solo marca el flag, NO hagas I2C aqui. */
    dev->event_pending = true;
}

bool lis2dw12_has_pending_event(lis2dw12_t *dev)
{
    return (dev != NULL) && dev->event_pending;
}

lis2dw12_status_t lis2dw12_process_event(lis2dw12_t *dev, lis2dw12_event_t *evt)
{
    if (dev == NULL) return LIS2DW12_ERR_PARAM;

    uint8_t src = 0;
    lis2dw12_status_t s = lis2dw12_read_wake_source(dev, &src);

    /* Limpia el flag haya error o no — si quedo set, la app puede reintentar */
    dev->event_pending = false;

    if (s != LIS2DW12_OK) return s;

    if (evt != NULL) {
        evt->raw_src  = src;
        evt->detected = (src & LIS2DW12_WU_SRC_WU_IA) != 0;
        evt->axis_x   = (src & LIS2DW12_WU_SRC_X) != 0;
        evt->axis_y   = (src & LIS2DW12_WU_SRC_Y) != 0;
        evt->axis_z   = (src & LIS2DW12_WU_SRC_Z) != 0;
    }
    return LIS2DW12_OK;
}
