/**
  ******************************************************************************
  * @file    vl53l0x.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Driver compacto VL53L0X (8 bits) sobre HAL I2C. Init estandar
  *          (config 2.8V, SPAD, tuning por defecto, calibracion de referencia)
  *          + medicion continua. Registros de 8 bits con auto-incremento.
  ******************************************************************************
  * Empresa: 0G IoT Solutions (previamente WND Mexico) - https://0giotsolutions.com/
  ******************************************************************************
  */

#include "vl53l0x.h"
#include "i2c.h"     /* hi2c1 (en realidad I2C2, ver i2c.h) */
#include "sys_app.h" /* APP_PPRINTF (diagnostico) */

#define VL53L0X_ADDR                 (0x29U << 1)   /* direccion HAL de 8 bits */
#define VL53L0X_TIMEOUT_MS           1000U

/* Registros usados */
#define REG_SYSRANGE_START           0x00
#define REG_SYSTEM_SEQUENCE_CONFIG   0x01
#define REG_SYSTEM_INTERRUPT_CFG     0x0A
#define REG_SYSTEM_INTERRUPT_CLEAR   0x0B
#define REG_RESULT_INTERRUPT_STATUS  0x13
#define REG_RESULT_RANGE_STATUS      0x14
#define REG_MSRC_CONFIG_CONTROL      0x60
#define REG_FINAL_RATE_RTN_LIMIT     0x44
#define REG_GPIO_HV_MUX_ACTIVE_HIGH  0x84
#define REG_SPAD_ENABLES_REF_0       0xB0
#define REG_DYN_SPAD_REF_EN_OFFSET   0x4F
#define REG_DYN_SPAD_NUM_REQ         0x4E
#define REG_GLOBAL_REF_EN_SELECT     0xB6
#define REG_MODEL_ID                 0xC0

static uint8_t stop_variable;

/* ------------------ acceso I2C (8 bits, auto-incremento) ------------------ */
static void wr8(uint8_t reg, uint8_t val)
{
  HAL_I2C_Mem_Write(&hi2c1, VL53L0X_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 50);
}
static uint8_t rd8(uint8_t reg)
{
  uint8_t v = 0;
  HAL_I2C_Mem_Read(&hi2c1, VL53L0X_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &v, 1, 50);
  return v;
}
static void wr16(uint8_t reg, uint16_t val)
{
  uint8_t b[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
  HAL_I2C_Mem_Write(&hi2c1, VL53L0X_ADDR, reg, I2C_MEMADD_SIZE_8BIT, b, 2, 50);
}
static uint16_t rd16(uint8_t reg)
{
  uint8_t b[2] = { 0 };
  HAL_I2C_Mem_Read(&hi2c1, VL53L0X_ADDR, reg, I2C_MEMADD_SIZE_8BIT, b, 2, 50);
  return ((uint16_t)b[0] << 8) | b[1];
}
static void rdN(uint8_t reg, uint8_t *dst, uint8_t n)
{
  HAL_I2C_Mem_Read(&hi2c1, VL53L0X_ADDR, reg, I2C_MEMADD_SIZE_8BIT, dst, n, 50);
}
static void wrN(uint8_t reg, uint8_t *src, uint8_t n)
{
  HAL_I2C_Mem_Write(&hi2c1, VL53L0X_ADDR, reg, I2C_MEMADD_SIZE_8BIT, src, n, 50);
}

/* --------------------------- sub-rutinas de init -------------------------- */
static uint8_t getSpadInfo(uint8_t *count, uint8_t *type_is_aperture)
{
  uint8_t tmp;
  uint32_t t0;

  wr8(0x80, 0x01); wr8(0xFF, 0x01); wr8(0x00, 0x00);
  wr8(0xFF, 0x06);
  wr8(0x83, rd8(0x83) | 0x04);
  wr8(0xFF, 0x07); wr8(0x81, 0x01);
  wr8(0x80, 0x01);
  wr8(0x94, 0x6b);
  wr8(0x83, 0x00);

  t0 = HAL_GetTick();
  while (rd8(0x83) == 0x00)
  {
    if ((HAL_GetTick() - t0) > VL53L0X_TIMEOUT_MS) return 0;
  }
  wr8(0x83, 0x01);
  tmp = rd8(0x92);
  *count = tmp & 0x7F;
  *type_is_aperture = (tmp >> 7) & 0x01;

  wr8(0x81, 0x00); wr8(0xFF, 0x06);
  wr8(0x83, rd8(0x83) & ~0x04);
  wr8(0xFF, 0x01); wr8(0x00, 0x01);
  wr8(0xFF, 0x00); wr8(0x80, 0x00);
  return 1;
}

static uint8_t performSingleRefCalibration(uint8_t vhv_init_byte)
{
  uint32_t t0;
  wr8(REG_SYSRANGE_START, 0x01 | vhv_init_byte);
  t0 = HAL_GetTick();
  while ((rd8(REG_RESULT_INTERRUPT_STATUS) & 0x07) == 0)
  {
    if ((HAL_GetTick() - t0) > VL53L0X_TIMEOUT_MS) return 0;
  }
  wr8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
  wr8(REG_SYSRANGE_START, 0x00);
  return 1;
}

static void loadTuningSettings(void)
{
  wr8(0xFF, 0x01); wr8(0x00, 0x00); wr8(0xFF, 0x00); wr8(0x09, 0x00);
  wr8(0x10, 0x00); wr8(0x11, 0x00); wr8(0x24, 0x01); wr8(0x25, 0xFF);
  wr8(0x75, 0x00); wr8(0xFF, 0x01); wr8(0x4E, 0x2C); wr8(0x48, 0x00);
  wr8(0x30, 0x20); wr8(0xFF, 0x00); wr8(0x30, 0x09); wr8(0x54, 0x00);
  wr8(0x31, 0x04); wr8(0x32, 0x03); wr8(0x40, 0x83); wr8(0x46, 0x25);
  wr8(0x60, 0x00); wr8(0x27, 0x00); wr8(0x50, 0x06); wr8(0x51, 0x00);
  wr8(0x52, 0x96); wr8(0x56, 0x08); wr8(0x57, 0x30); wr8(0x61, 0x00);
  wr8(0x62, 0x00); wr8(0x64, 0x00); wr8(0x65, 0x00); wr8(0x66, 0xA0);
  wr8(0xFF, 0x01); wr8(0x22, 0x32); wr8(0x47, 0x14); wr8(0x49, 0xFF);
  wr8(0x4A, 0x00); wr8(0xFF, 0x00); wr8(0x7A, 0x0A); wr8(0x7B, 0x00);
  wr8(0x78, 0x21); wr8(0xFF, 0x01); wr8(0x23, 0x34); wr8(0x42, 0x00);
  wr8(0x44, 0xFF); wr8(0x45, 0x26); wr8(0x46, 0x05); wr8(0x40, 0x40);
  wr8(0x0E, 0x06); wr8(0x20, 0x1A); wr8(0x43, 0x40); wr8(0xFF, 0x00);
  wr8(0x34, 0x03); wr8(0x35, 0x44); wr8(0xFF, 0x01); wr8(0x31, 0x04);
  wr8(0x4B, 0x09); wr8(0x4C, 0x05); wr8(0x4D, 0x04); wr8(0xFF, 0x00);
  wr8(0x44, 0x00); wr8(0x45, 0x20); wr8(0x47, 0x08); wr8(0x48, 0x28);
  wr8(0x67, 0x00); wr8(0x70, 0x04); wr8(0x71, 0x01); wr8(0x72, 0xFE);
  wr8(0x76, 0x00); wr8(0x77, 0x00); wr8(0xFF, 0x01); wr8(0x0D, 0x01);
  wr8(0xFF, 0x00); wr8(0x80, 0x01); wr8(0x01, 0xF8); wr8(0xFF, 0x01);
  wr8(0x8E, 0x01); wr8(0x00, 0x01); wr8(0xFF, 0x00); wr8(0x80, 0x00);
}

/* ------------- Presupuesto de medicion (timeouts de pre/final range) -------
   Sin esto la secuencia 0xE8 nunca termina. Portado de la implementacion
   publica de Pololu (MIT). */
#define REG_PRE_RANGE_VCSEL_PERIOD    0x50
#define REG_FINAL_RANGE_VCSEL_PERIOD  0x70
#define REG_MSRC_TIMEOUT              0x46
#define REG_PRE_RANGE_TIMEOUT_HI      0x51
#define REG_FINAL_RANGE_TIMEOUT_HI    0x71

static uint32_t calcMacroPeriod(uint8_t vcsel_pclks)
{
  return ((2304UL * (uint32_t)vcsel_pclks * 1655UL) + 500UL) / 1000UL;
}
static uint16_t decodeTimeout(uint16_t reg_val)
{
  return (uint16_t)(((uint32_t)(reg_val & 0x00FF) << ((reg_val & 0xFF00) >> 8)) + 1);
}
static uint16_t encodeTimeout(uint32_t timeout_mclks)
{
  uint32_t ls = 0; uint16_t ms = 0;
  if (timeout_mclks > 0)
  {
    ls = timeout_mclks - 1;
    while ((ls & 0xFFFFFF00UL) > 0) { ls >>= 1; ms++; }
    return (uint16_t)((ms << 8) | (ls & 0xFF));
  }
  return 0;
}
static uint32_t mclksToUs(uint16_t mclks, uint8_t vcsel_pclks)
{
  uint32_t macro_ns = calcMacroPeriod(vcsel_pclks);
  return ((uint32_t)mclks * macro_ns + 500UL) / 1000UL;
}
static uint32_t usToMclks(uint32_t us, uint8_t vcsel_pclks)
{
  uint32_t macro_ns = calcMacroPeriod(vcsel_pclks);
  return (((us * 1000UL) + (macro_ns / 2)) / macro_ns);
}
static uint8_t vcselPeriod(uint8_t reg)
{
  return (uint8_t)(((rd8(reg)) + 1) << 1);
}

static void setTimingBudget(uint32_t budget_us)
{
  uint8_t seq = rd8(REG_SYSTEM_SEQUENCE_CONFIG);
  uint8_t tcc = (seq >> 4) & 1, dss = (seq >> 3) & 1, msrc = (seq >> 2) & 1;
  uint8_t pre = (seq >> 6) & 1, fin = (seq >> 7) & 1;

  uint8_t  pre_vcsel = vcselPeriod(REG_PRE_RANGE_VCSEL_PERIOD);
  uint8_t  fin_vcsel = vcselPeriod(REG_FINAL_RANGE_VCSEL_PERIOD);
  uint16_t msrc_mclks = (uint16_t)rd8(REG_MSRC_TIMEOUT) + 1;
  uint32_t msrc_us = mclksToUs(msrc_mclks, pre_vcsel);
  uint16_t pre_mclks = decodeTimeout(rd16(REG_PRE_RANGE_TIMEOUT_HI));
  uint32_t pre_us = mclksToUs(pre_mclks, pre_vcsel);

  const uint32_t StartOv = 1910, EndOv = 960, MsrcOv = 660, TccOv = 590,
                 DssOv = 690, PreOv = 660, FinOv = 550;
  uint32_t used = StartOv + EndOv;

  if (tcc)  used += msrc_us + TccOv;
  if (dss)  used += 2 * (msrc_us + DssOv);
  else if (msrc) used += msrc_us + MsrcOv;
  if (pre)  used += pre_us + PreOv;

  if (fin)
  {
    uint32_t fin_us, fin_mclks;
    used += FinOv;
    if (used >= budget_us) return;      /* presupuesto demasiado chico */
    fin_us = budget_us - used;
    fin_mclks = usToMclks(fin_us, fin_vcsel);
    if (pre) fin_mclks += pre_mclks;
    wr16(REG_FINAL_RANGE_TIMEOUT_HI, encodeTimeout(fin_mclks));
  }
}

/* -------------------------------- API ------------------------------------ */
uint8_t VL53L0X_Init(void)
{
  uint8_t spad_count = 0, spad_is_aperture = 0;
  uint8_t ref_spad_map[6];
  uint8_t first_spad, spads_enabled, i;

  if (rd8(REG_MODEL_ID) != 0xEE) return 0;   /* no es un VL53L0X */

  /* 1. Config a 2.8V / modo estandar y captura de stop_variable */
  wr8(0x88, 0x00);
  wr8(0x80, 0x01); wr8(0xFF, 0x01); wr8(0x00, 0x00);
  stop_variable = rd8(0x91);
  wr8(0x00, 0x01); wr8(0xFF, 0x00); wr8(0x80, 0x00);

  /* 2. Desactiva chequeos de rate MSRC y pre-range */
  wr8(REG_MSRC_CONFIG_CONTROL, rd8(REG_MSRC_CONFIG_CONTROL) | 0x12);

  /* 3. Limite de signal rate final = 0.25 MCPS */
  wr16(REG_FINAL_RATE_RTN_LIMIT, (uint16_t)(0.25f * (1 << 7)));

  wr8(REG_SYSTEM_SEQUENCE_CONFIG, 0xFF);

  /* 4. SPAD info + configuracion del mapa de SPADs de referencia */
  if (!getSpadInfo(&spad_count, &spad_is_aperture)) return 0;

  rdN(REG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

  wr8(0xFF, 0x01);
  wr8(REG_DYN_SPAD_REF_EN_OFFSET, 0x00);
  wr8(REG_DYN_SPAD_NUM_REQ, 0x2C);
  wr8(0xFF, 0x00);
  wr8(REG_GLOBAL_REF_EN_SELECT, 0xB4);

  first_spad = spad_is_aperture ? 12 : 0;
  spads_enabled = 0;
  for (i = 0; i < 48; i++)
  {
    if (i < first_spad || spads_enabled == spad_count)
    {
      ref_spad_map[i / 8] &= ~(1 << (i % 8));
    }
    else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x01)
    {
      spads_enabled++;
    }
  }
  wrN(REG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

  /* 5. Ajustes de tuning por defecto */
  loadTuningSettings();

  /* 6. Config de interrupcion (data-ready por GPIO, activo bajo) */
  wr8(REG_SYSTEM_INTERRUPT_CFG, 0x04);
  wr8(REG_GPIO_HV_MUX_ACTIVE_HIGH, rd8(REG_GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
  wr8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

  wr8(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);

  /* 7. Calibracion de referencia (VHV + fase) */
  wr8(REG_SYSTEM_SEQUENCE_CONFIG, 0x01);
  if (!performSingleRefCalibration(0x40)) return 0;
  wr8(REG_SYSTEM_SEQUENCE_CONFIG, 0x02);
  if (!performSingleRefCalibration(0x00)) return 0;
  wr8(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);

  /* 8. Fija el presupuesto de medicion (~33 ms). SIN esto la secuencia 0xE8
     no termina y la lectura hace timeout. */
  setTimingBudget(33000);

  APP_PPRINTF("[VL53] init: seq=0x%02X finTimeout(0x71)=0x%04X preVcsel=%u finVcsel=%u\r\n",
              (unsigned)rd8(REG_SYSTEM_SEQUENCE_CONFIG),
              (unsigned)rd16(REG_FINAL_RANGE_TIMEOUT_HI),
              (unsigned)vcselPeriod(REG_PRE_RANGE_VCSEL_PERIOD),
              (unsigned)vcselPeriod(REG_FINAL_RANGE_VCSEL_PERIOD));

  /* Listo. La medicion se dispara por lectura (single-shot). */
  return 1;
}

uint16_t VL53L0X_ReadRangeContinuousMillimeters(void)
{
  uint32_t t0;
  uint16_t range;

  /* Secuencia stop_variable antes de cada disparo */
  wr8(0x80, 0x01); wr8(0xFF, 0x01); wr8(0x00, 0x00);
  wr8(0x91, stop_variable);
  wr8(0x00, 0x01); wr8(0xFF, 0x00); wr8(0x80, 0x00);

  /* Disparo unico */
  wr8(REG_SYSRANGE_START, 0x01);

  /* Espera a que el bit de arranque se limpie (medicion iniciada).
     Sentinela 0xFFFE = se atoro aqui (la medicion no arranca). */
  t0 = HAL_GetTick();
  while (rd8(REG_SYSRANGE_START) & 0x01)
  {
    if ((HAL_GetTick() - t0) > VL53L0X_TIMEOUT_MS) return 0xFFFE;
  }

  /* Espera a que el dato este listo.
     Sentinela 0xFFFD = arranco pero nunca marco dato listo. */
  t0 = HAL_GetTick();
  while ((rd8(REG_RESULT_INTERRUPT_STATUS) & 0x07) == 0)
  {
    if ((HAL_GetTick() - t0) > VL53L0X_TIMEOUT_MS)
    {
      APP_PPRINTF("[VL53] timeout: rangeStatus=0x%02X intr=0x%02X start=0x%02X modelID(0xC0)=0x%02X\r\n",
                  (unsigned)rd8(REG_RESULT_RANGE_STATUS),
                  (unsigned)rd8(REG_RESULT_INTERRUPT_STATUS),
                  (unsigned)rd8(REG_SYSRANGE_START),
                  (unsigned)rd8(REG_MODEL_ID));
      return 0xFFFD;
    }
  }

  /* Distancia en RESULT_RANGE_STATUS + 10 (0x1E), 2 bytes */
  range = rd16(REG_RESULT_RANGE_STATUS + 10);
  wr8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
  return range;
}
