 /**
  ******************************************************************************
  * @file    distance_app.c
  * @author  Yahir Flores - 0G IoT Solutions
  * @brief   Lectura periodica de 2 sensores VL53L0X/VL53L1X detras de un
  *          multiplexor I2C PCA9548A, y envio de las distancias por Sigfox.
  *
  *          Flujo:
  *            UTIL_TIMER (periodico) --> SetTask(CFG_SEQ_Task_DistanceTx)
  *            --> Distance_Task():
  *                  Mux_Select(A) -> lee sensor A
  *                  Mux_Select(B) -> lee sensor B
  *                  arma payload -> SIGFOX_API_send_frame() (TX compartido)
  *
  *          Solo un canal del mux activo a la vez: ambos VL53 comparten la
  *          direccion 0x29, y el PCA9548A es justo lo que evita el choque.
  ******************************************************************************
  * Fecha:   Julio 2026
  * Version: 1.0
  * Empresa: 0G IoT Solutions (previamente WND Mexico)
  *          https://0giotsolutions.com/
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "distance_app.h"
#include "app_features.h"

#if USE_DISTANCE_APP   /* ==== Modulo completo condicionado por feature flag ==== */

#include <stdint.h>
#include "i2c.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "stm32_lpm.h"
#include "utilities_def.h"
#include "sys_app.h"
#include "sigfox_types.h"
#include "st_sigfox_api.h"
#include "vl53l0x.h"          /* driver propio del VL53L0X (8 bits) */

#if DIST_HAVE_VL53_ULD
#include "VL53L1X_api.h"      /* ULD oficial de ST (STSW-IMG009). Copialo a Sigfox/App/ */
#endif

/* Escaner I2C de diagnostico al arranque. Ponlo en 0 cuando el bus ya funcione. */
#ifndef DIST_I2C_SCAN
#define DIST_I2C_SCAN 1
#endif

/* Auto-test de pines: hace toggle de SCL/SDA como GPIO para verificar con
   multimetro cual pin fisico responde. Ponlo en 1 para diagnosticar, 0 para
   operar normal. Corre ANTES de inicializar el I2C. */
#ifndef DIST_PIN_TEST
#define DIST_PIN_TEST 0
#endif

/* 1 = UN solo sensor conectado DIRECTO al bus, SIN mux (pruebas): salta
   Mux_Select y le habla al VL53 en 0x29 directamente. Ambos "canales" leen
   el mismo sensor. 0 = operacion normal con el PCA9548A. */
#ifndef DIST_DIRECT_ONE_SENSOR
#define DIST_DIRECT_ONE_SENSOR 1
#endif

/* 1 = usa el driver VL53L0X (registros de 8 bits). Es el chip que tienes.
   Tiene prioridad sobre DIST_HAVE_VL53_ULD (que es para el VL53L1X). */
#ifndef DIST_USE_VL53L0X
#define DIST_USE_VL53L0X 1
#endif

/* === Estado === */
static UTIL_TIMER_Object_t DistTimer;

#if (DIST_PAYLOAD_MODE != DIST_MODE_MM)
/* Estado de presencia con histeresis (solo modos FLAGS / BOTH). */
static uint8_t  presA       = 0;         /* 1 = algo cerca del sensor A */
static uint8_t  presB       = 0;         /* 1 = algo cerca del sensor B */
static uint8_t  lastTxFlags = 0xFF;      /* flags del ultimo TX (0xFF = ninguno) */
static uint32_t sampleCount = 0;         /* ciclos de timer transcurridos */
static uint32_t lastTxSample = 0;        /* sampleCount del ultimo TX */
static uint32_t txToday        = 0;      /* uplinks enviados en la ventana de 24 h */
static uint32_t dayStartSample = 0;      /* sampleCount al inicio de la ventana */

/* Conversion de tiempos a numero de ciclos del timer (>=1 para no dividir a 0). */
#define TX_MIN_SAMPLES    ((DIST_MIN_TX_INTERVAL_MS / DIST_SAMPLE_MS) ? \
                           (DIST_MIN_TX_INTERVAL_MS / DIST_SAMPLE_MS) : 1U)
#define HEARTBEAT_SAMPLES ((DIST_HEARTBEAT_MS / DIST_SAMPLE_MS) ? \
                           (DIST_HEARTBEAT_MS / DIST_SAMPLE_MS) : 1U)
/* Ciclos de timer en una ventana de 24 h (para el tope diario de uplinks). */
#define SAMPLES_PER_DAY   ((24UL * 60UL * 60UL * 1000UL) / DIST_SAMPLE_MS)
#endif

/* === Prototipos privados === */
static void      Distance_Task(void);
static void      OnDistanceTimer(void *context);
static HAL_StatusTypeDef Mux_Select(uint8_t ch);
static uint16_t  Distance_ReadOne(uint8_t ch);
static void      Sensor_Setup(uint8_t ch);
static void      Distance_SendPayload(uint8_t *payload, uint8_t len);
#if (DIST_PAYLOAD_MODE != DIST_MODE_MM)
static uint8_t   Presence_Update(uint8_t prev, uint16_t mm);
#endif
#if DIST_I2C_SCAN
static void      Distance_I2C_Scan(void);
static void      Distance_I2C_Probe(uint8_t addr7);
#endif
#if DIST_PIN_TEST
static void      Distance_PinTest(void);
#endif

/* ============================================================================
 *  API PUBLICA
 * ==========================================================================*/
void Distance_Init(void)
{
#if DIST_PIN_TEST
  /* Diagnostico de pines ANTES de configurar el I2C. */
  Distance_PinTest();
#endif

  /* 1. Bus I2C1 (init perezoso: solo se enciende en la build de distancia). */
  MX_I2C1_Init();

  /* El I2C no sobrevive STOP2 (bajo consumo): las lecturas periodicas fallan
     tras dormir. Evita STOP/OFF mientras la app de distancia esta activa,
     asi el periferico I2C sigue vivo entre muestras. (Optimizable luego:
     re-inicializar el I2C al despertar en lugar de bloquear el sleep.) */
  UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
  UTIL_LPM_SetOffMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);

#if DIST_I2C_SCAN
  /* Diagnostico: que hay realmente en el bus antes de hablar con los sensores. */
  Distance_I2C_Scan();
#endif

  /* 2. Registra la tarea en el sequencer (mismo patron que Task_ButtonTx). */
  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_DistanceTx), UTIL_SEQ_RFU, Distance_Task);

  /* 3. Configura cada sensor seleccionando su canal en el mux. */
  Sensor_Setup(DIST_SENSOR_A_CH);
  Sensor_Setup(DIST_SENSOR_B_CH);

  /* 4. Timer periodico -> dispara la tarea (nada de busy-loop; respeta LPM).
        En modo presencia el timer es rapido (muestreo); el TX va por evento. */
  UTIL_TIMER_Create(&DistTimer, DIST_TIMER_MS, UTIL_TIMER_PERIODIC,
                    OnDistanceTimer, NULL);
  UTIL_TIMER_Start(&DistTimer);

  APP_PPRINTF("\r\nDISTANCE APP LISTA:\r\n");
  APP_PPRINTF("  Mux PCA9548A @ 0x%02X | Sensores en canales %u y %u\r\n",
              (unsigned)PCA9548A_ADDR_7B,
              (unsigned)DIST_SENSOR_A_CH, (unsigned)DIST_SENSOR_B_CH);
#if (DIST_PAYLOAD_MODE == DIST_MODE_MM)
  APP_PPRINTF("  Modo: mm crudo | Periodo TX: %u s | Replicas: %u | ULD: %s\r\n\r\n",
              (unsigned)(DIST_PERIOD_MS / 1000U),
              (unsigned)DIST_TX_REPLICAS,
              (DIST_HAVE_VL53_ULD ? "si" : "no (bring-up)"));
#else
  APP_PPRINTF("  Puerta: CERRADA < %u mm, ABIERTA > %u mm (histeresis) | Muestreo: %u s\r\n",
              (unsigned)DIST_NEAR_MM, (unsigned)DIST_FAR_MM,
              (unsigned)(DIST_SAMPLE_MS / 1000U));
#if (DIST_HEARTBEAT_MS > 0U)
  APP_PPRINTF("  TX por evento (guard %u s) + heartbeat %u h\r\n\r\n",
              (unsigned)(DIST_MIN_TX_INTERVAL_MS / 1000U),
              (unsigned)(DIST_HEARTBEAT_MS / 3600000U));
#else
  APP_PPRINTF("  TX SOLO por evento abrir/cerrar (sin periodicidad)\r\n\r\n");
#endif
#endif
}

/* ============================================================================
 *  LOGICA INTERNA
 * ==========================================================================*/

/**
  * @brief  Callback del timer: agenda la tarea en el sequencer (contexto ISR).
  */
static void OnDistanceTimer(void *context)
{
  (void)context;
  UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_DistanceTx), CFG_SEQ_Prio_0);
}

/**
  * @brief  Selecciona un unico canal del PCA9548A (bitmask de 1 byte).
  */
static HAL_StatusTypeDef Mux_Select(uint8_t ch)
{
#if DIST_DIRECT_ONE_SENSOR
  (void)ch;
  return HAL_OK;   /* sin mux: no hay canal que seleccionar, hablamos directo */
#else
  uint8_t mask = (uint8_t)(1U << ch);
  return HAL_I2C_Master_Transmit(&hi2c1, PCA9548A_ADDR, &mask, 1, 10);
#endif
}

#if DIST_I2C_SCAN
/**
  * @brief  Barre las direcciones 0x08..0x77 y loguea las que dan ACK.
  */
static void Distance_I2C_ScanRange(const char *tag)
{
  uint8_t found = 0U;
  for (uint8_t addr = 0x08U; addr <= 0x77U; addr++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2U, 5U) == HAL_OK)
    {
      APP_PPRINTF("[I2C SCAN] %s ACK 0x%02X\r\n", tag, (unsigned)addr);
      found++;
    }
  }
  if (found == 0U)
  {
    APP_PPRINTF("[I2C SCAN] %s nada respondio\r\n", tag);
  }
}

/**
  * @brief  Diagnostico completo: escanea el bus principal (debe salir el mux
  *         0x70) y luego cada canal del mux (debe salir el VL53 0x29 + 0x70).
  *         Temporal: pon DIST_I2C_SCAN en 0 cuando el bus ya funcione.
  */
static void Distance_I2C_Scan(void)
{
  APP_PPRINTF("\r\n[I2C SCAN] bus principal (esperado: mux 0x70)\r\n");
  Distance_I2C_ScanRange("bus ");

  for (uint8_t ch = 0U; ch < 2U; ch++)
  {
    if (Mux_Select(ch) == HAL_OK)
    {
      APP_PPRINTF("[I2C SCAN] canal %u (esperado: VL53 0x29 + mux 0x70)\r\n",
                  (unsigned)ch);
      Distance_I2C_ScanRange("ch  ");
    }
    else
    {
      APP_PPRINTF("[I2C SCAN] canal %u: mux 0x70 no responde\r\n", (unsigned)ch);
    }
  }

  /* Sondeo puntual: dice POR QUE falla (NACK = bus OK pero nadie contesta;
     TIMEOUT = el I2C no esta generando reloj / linea atorada). */
  Distance_I2C_Probe(0x29);   /* VL53 */
  Distance_I2C_Probe(0x70);   /* mux */
  APP_PPRINTF("\r\n");
}

static void Distance_I2C_Probe(uint8_t addr7)
{
  uint8_t dummy = 0;
  HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                                 &dummy, 1, 20U);
  uint32_t err = HAL_I2C_GetError(&hi2c1);

  if (st == HAL_OK)
  {
    APP_PPRINTF("[I2C PROBE] 0x%02X -> ACK! responde\r\n", (unsigned)addr7);
  }
  else if (err & HAL_I2C_ERROR_AF)
  {
    APP_PPRINTF("[I2C PROBE] 0x%02X -> NACK (bus OK, nadie en esa direccion)\r\n",
                (unsigned)addr7);
  }
  else if (err & HAL_I2C_ERROR_TIMEOUT)
  {
    APP_PPRINTF("[I2C PROBE] 0x%02X -> TIMEOUT (I2C sin reloj o linea atorada)\r\n",
                (unsigned)addr7);
  }
  else
  {
    APP_PPRINTF("[I2C PROBE] 0x%02X -> fallo, err=0x%08lX\r\n",
                (unsigned)addr7, (unsigned long)err);
  }
}
#endif

#if DIST_PIN_TEST
/**
  * @brief  Auto-test de pines: PB8 (SCL) y PB7 (SDA) como salidas push-pull,
  *         alternando alto/bajo con 3 s de pausa para medir con multimetro.
  *         Confirma cual pin fisico del header responde (y que no esten
  *         cruzados ni en el pin equivocado).
  */
static void Distance_PinTest(void)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;

  g.Pin = I2C1_SCL_Pin;  HAL_GPIO_Init(I2C1_SCL_GPIO_Port, &g);
  g.Pin = I2C1_SDA_Pin;  HAL_GPIO_Init(I2C1_SDA_GPIO_Port, &g);

  APP_PPRINTF("\r\n[PIN TEST] mide con multimetro; cada pin cambia cada 3 s:\r\n");

  for (uint8_t i = 0; i < 2U; i++)
  {
    APP_PPRINTF("[PIN TEST] SCL = PA12 = Arduino \"SCL\" (D15) -> ALTO (~3.3V)\r\n");
    HAL_GPIO_WritePin(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_SET);
    HAL_Delay(3000);
    APP_PPRINTF("[PIN TEST] SCL = PA12 = Arduino \"SCL\" (D15) -> BAJO (~0V)\r\n");
    HAL_GPIO_WritePin(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_RESET);
    HAL_Delay(3000);

    APP_PPRINTF("[PIN TEST] SDA = PA11 = Arduino \"SDA\" (D14) -> ALTO (~3.3V)\r\n");
    HAL_GPIO_WritePin(I2C1_SDA_GPIO_Port, I2C1_SDA_Pin, GPIO_PIN_SET);
    HAL_Delay(3000);
    APP_PPRINTF("[PIN TEST] SDA = PA11 = Arduino \"SDA\" (D14) -> BAJO (~0V)\r\n");
    HAL_GPIO_WritePin(I2C1_SDA_GPIO_Port, I2C1_SDA_Pin, GPIO_PIN_RESET);
    HAL_Delay(3000);
  }
  APP_PPRINTF("[PIN TEST] fin. Pon DIST_PIN_TEST en 0 para operar normal.\r\n\r\n");
}
#endif

/**
  * @brief  Configura un sensor (tras seleccionar su canal en el mux).
  */
static void Sensor_Setup(uint8_t ch)
{
  if (Mux_Select(ch) != HAL_OK)
  {
    APP_PPRINTF("[DIST] Mux canal %u: sin ACK (revisa cableado/pull-ups)\r\n",
                (unsigned)ch);
    return;
  }

#if DIST_USE_VL53L0X
  if (VL53L0X_Init())
  {
    APP_PPRINTF("[DIST] Canal %u: VL53L0X iniciado OK\r\n", (unsigned)ch);
  }
  else
  {
    APP_PPRINTF("[DIST] Canal %u: VL53L0X init FALLO\r\n", (unsigned)ch);
  }
#elif DIST_HAVE_VL53_ULD
  uint8_t boot = 0, tries = 0;
  do {
    VL53L1X_BootState(VL53_I2C_ADDR, &boot);
    if (!boot) HAL_Delay(2);
  } while (!boot && ++tries < 50);

  VL53L1X_SensorInit(VL53_I2C_ADDR);
  VL53L1X_SetDistanceMode(VL53_I2C_ADDR, 2);          /* 2 = long range */
  VL53L1X_SetTimingBudgetInMs(VL53_I2C_ADDR, 50);
  VL53L1X_SetInterMeasurementInMs(VL53_I2C_ADDR, 100);
  VL53L1X_StartRanging(VL53_I2C_ADDR);
  APP_PPRINTF("[DIST] Canal %u: VL53L1X iniciado\r\n", (unsigned)ch);
#else
  /* Bring-up: identifica el chip probando los dos esquemas de registro.
     VL53L1X -> reg 0x010F, direccion de 16 bits, deberia dar 0xEA 0xCC
     VL53L0X -> reg 0xC0,   direccion de 8 bits,  deberia dar 0xEE (regs de 8b) */
  uint8_t id16[2] = {0};
  uint8_t id8 = 0;
  HAL_I2C_Mem_Read(&hi2c1, VL53_I2C_ADDR, 0x010F, I2C_MEMADD_SIZE_16BIT, id16, 2, 100);
  HAL_I2C_Mem_Read(&hi2c1, VL53_I2C_ADDR, 0x00C0, I2C_MEMADD_SIZE_8BIT,  &id8, 1, 100);
  APP_PPRINTF("[DIST] Canal %u: L1X(0x010F/16b)=0x%02X%02X  L0X(0xC0/8b)=0x%02X\r\n",
              (unsigned)ch, id16[0], id16[1], (unsigned)id8);
#endif
}

/**
  * @brief  Lee la distancia (mm) de un sensor tras conmutar su canal.
  * @return distancia en mm (0xFFFF si error / no disponible).
  */
static uint16_t Distance_ReadOne(uint8_t ch)
{
  if (Mux_Select(ch) != HAL_OK)
  {
    return 0xFFFFU;
  }

#if DIST_USE_VL53L0X
  return VL53L0X_ReadRangeContinuousMillimeters();
#elif DIST_HAVE_VL53_ULD
  uint8_t  ready = 0, tries = 0;
  uint16_t mm = 0xFFFFU;
  do {
    VL53L1X_CheckForDataReady(VL53_I2C_ADDR, &ready);
    if (!ready) HAL_Delay(2);
  } while (!ready && ++tries < 60);

  if (ready)
  {
    VL53L1X_GetDistance(VL53_I2C_ADDR, &mm);
    VL53L1X_ClearInterrupt(VL53_I2C_ADDR);
  }
  return mm;
#else
  /* Bring-up: sin ULD no hay medida real; devuelve el Model ID como sello. */
  uint8_t id[2] = {0};
  if (HAL_I2C_Mem_Read(&hi2c1, VL53_I2C_ADDR, 0x010F, I2C_MEMADD_SIZE_16BIT,
                       id, 2, 100) == HAL_OK)
  {
    return (uint16_t)((id[0] << 8) | id[1]);
  }
  return 0xFFFFU;
#endif
}

/**
  * @brief  Envia el payload por Sigfox (TX compartido) y loguea el resultado.
  */
static void Distance_SendPayload(uint8_t *payload, uint8_t len)
{
#if DIST_TX_ENABLE
  uint8_t dl_msg[8] = { 0 };
  sfx_error_t err = SIGFOX_API_send_frame((sfx_u8 *)payload, (sfx_u8)len,
                                          dl_msg, (sfx_u8)DIST_TX_REPLICAS,
                                          SFX_FALSE);
  if (err == SFX_ERR_NONE)
  {
    APP_PPRINTF("[DIST] << TX OK (%u bytes)\r\n\r\n", (unsigned)len);
  }
  else
  {
    APP_PPRINTF("[DIST] << TX ERROR 0x%04X\r\n\r\n", err);
  }
#else
  (void)payload; (void)len;
  APP_PPRINTF("[DIST] (TX deshabilitado, solo lectura)\r\n\r\n");
#endif
}

#if (DIST_PAYLOAD_MODE != DIST_MODE_MM)
/**
  * @brief  Actualiza el estado de presencia de un sensor aplicando histeresis.
  *         prev=0 -> pasa a 1 solo si mm < NEAR; prev=1 -> pasa a 0 solo si
  *         mm > FAR. Dentro de la banda [NEAR, FAR] mantiene el estado.
  *         Sin dato (0xFFFF) conserva el estado previo (no genera falso cambio).
  */
static uint8_t Presence_Update(uint8_t prev, uint16_t mm)
{
  if (mm == 0xFFFFU)                    return prev; /* sin dato: no cambia   */
  if (prev == 0U && mm < DIST_NEAR_MM)  return 1U;   /* cruzo a "cerca"       */
  if (prev == 1U && mm > DIST_FAR_MM)   return 0U;   /* cruzo a "lejos"       */
  return prev;                                       /* banda muerta          */
}
#endif

/**
  * @brief  Tarea del sequencer: lee ambos sensores y (segun el modo) transmite.
  */
static void Distance_Task(void)
{
  uint16_t dA = Distance_ReadOne(DIST_SENSOR_A_CH);
  uint16_t dB = Distance_ReadOne(DIST_SENSOR_B_CH);

#if (DIST_PAYLOAD_MODE == DIST_MODE_MM)
  /* ---- Modo mm crudo: 4 bytes, envio en cada ciclo del timer ------------- */
  APP_PPRINTF("[DIST] A=%u mm  B=%u mm\r\n", (unsigned)dA, (unsigned)dB);
  uint8_t payload[4];
  payload[0] = (uint8_t)(dA >> 8); payload[1] = (uint8_t)(dA & 0xFF);
  payload[2] = (uint8_t)(dB >> 8); payload[3] = (uint8_t)(dB & 0xFF);
  Distance_SendPayload(payload, sizeof(payload));

#else
  /* ---- Modo presencia: histeresis + envio por evento --------------------- */
  sampleCount++;
  presA = Presence_Update(presA, dA);
  presB = Presence_Update(presB, dB);

  /* bit0 = sensor A cerca ; bit1 = sensor B cerca */
  uint8_t flags = (uint8_t)((presA ? 0x01U : 0U) | (presB ? 0x02U : 0U));

  /* Reinicia la ventana de 24 h del presupuesto de uplinks. */
  if ((sampleCount - dayStartSample) >= SAMPLES_PER_DAY)
  {
    txToday = 0;
    dayStartSample = sampleCount;
  }

  uint32_t sinceTx   = sampleCount - lastTxSample;
  uint8_t  changed   = (flags != lastTxFlags);
  uint8_t  guardOk   = (sinceTx >= TX_MIN_SAMPLES);
#if (DIST_HEARTBEAT_MS > 0U)
  uint8_t  heartbeat = (sinceTx >= HEARTBEAT_SAMPLES);
#else
  uint8_t  heartbeat = 0U;   /* sin heartbeat: transmite SOLO en cambio de estado */
#endif
  uint8_t  wantTx    = (uint8_t)((changed && guardOk) || heartbeat);
  uint8_t  budgetOk  = (uint8_t)((DIST_MAX_TX_PER_DAY == 0U) ||
                                 (txToday < DIST_MAX_TX_PER_DAY));

  APP_PPRINTF("[DIST] A=%u mm(%u)  B=%u mm(%u)  flags=0x%02X  (tx hoy %u/%u)\r\n",
              (unsigned)dA, (unsigned)presA,
              (unsigned)dB, (unsigned)presB, (unsigned)flags,
              (unsigned)txToday, (unsigned)DIST_MAX_TX_PER_DAY);

  /* Transmite SOLO por evento: el estado cambio (respetando el guard minimo)
     o vencio el heartbeat. Ademas, nunca por encima del tope diario. Si el
     guard aun no vence, el cambio queda pendiente y se enviara en cuanto pueda. */
  if (wantTx && budgetOk)
  {
#if (DIST_PAYLOAD_MODE == DIST_MODE_BOTH)
    uint8_t payload[5];
    payload[0] = flags;
    payload[1] = (uint8_t)(dA >> 8); payload[2] = (uint8_t)(dA & 0xFF);
    payload[3] = (uint8_t)(dB >> 8); payload[4] = (uint8_t)(dB & 0xFF);
#else /* DIST_MODE_FLAGS */
    uint8_t payload[1];
    payload[0] = flags;
#endif
    APP_PPRINTF("[DIST] >> TX evento: sensor A=%s  B=%s  (flags=0x%02X)\r\n",
                (flags & 0x01) ? "CERRADA" : "ABIERTA",
                (flags & 0x02) ? "CERRADA" : "ABIERTA", (unsigned)flags);
    Distance_SendPayload(payload, (uint8_t)sizeof(payload));
    lastTxFlags  = flags;
    lastTxSample = sampleCount;
    txToday++;
  }
  else if (wantTx && !budgetOk)
  {
    /* Presupuesto agotado: acepta el estado para no reintentar en bucle. */
    APP_PPRINTF("[DIST] tope diario alcanzado (%u), TX omitido\r\n",
                (unsigned)DIST_MAX_TX_PER_DAY);
    lastTxFlags  = flags;
    lastTxSample = sampleCount;
  }
#endif
}

#endif /* USE_DISTANCE_APP */
