# Programa Sensor de Distancia / Puerta — VL53L0X (NUCLEO-WL55JC → Sigfox)

Añade al firmware `Sigfox_AT_Slave` V1.5.0 la lectura de un sensor ToF **VL53L0X** por I2C y la detección **puerta abierta / cerrada** con histéresis, con envío Sigfox **solo por evento**. Mantiene en paralelo la interfaz AT.

Backend Sigfox de **0G IoT Solutions** (previamente WND México) — https://0giotsolutions.com/

---

## Contenido de la carpeta

```
Programa_Sensor_Distancia_VL53L0X/
├── README.md
└── src/
    ├── app_features.h            Selector de apps (feature flags)         [NUEVO -> Core/Inc]
    ├── i2c.h / i2c.c             Periférico I2C2 (bus del VL53)           [NUEVO -> Core/Inc, Core/Src]
    ├── distance_app.h / .c       Lógica de distancia / puerta             [NUEVO -> Sigfox/App]
    ├── vl53l0x.h / .c            Driver propio VL53L0X (8 bits)           [NUEVO -> Sigfox/App]
    ├── vl53l1_platform.h / .c    Shim ULD VL53L1X (opcional, no usado)    [NUEVO -> Sigfox/App]
    ├── sgfx_app.c.patch          Llamada Distance_Init() en Sigfox_Init() [MOD]
    ├── utilities_def.h.patch     ID de tarea CFG_SEQ_Task_DistanceTx      [MOD]
    ├── stm32wlxx_hal_conf.h.patch Habilita HAL_I2C_MODULE_ENABLED         [MOD]
    └── dot_project.patch         Recursos enlazados (.c + driver I2C)     [MOD]
```

---

## Hardware / cableado

Sensor **VL53L0X** (ToF, registros de 8 bits, WHO_AM_I `0xC0 = 0xEE`) por **I2C2**:

| Señal sensor | Pin Nucleo | Header |
|--------------|-----------|--------|
| SCL | **PA12** | Arduino D15 ("SCL") |
| SDA | **PA11** | Arduino D14 ("SDA") |
| VIN | 3.3 V | |
| GND | GND | |
| XSHUT | 3.3 V | (habilita el sensor) |

Pull-ups **4.7 kΩ a 3.3 V** en SDA y SCL. Se usó PA11/PA12 porque **PB7 (D0) está cargado por el ST-LINK**.

> El multiplexor **PCA9548A** (para 2 sensores) resultó **dañado**; este programa corre con **un sensor directo** (`DIST_DIRECT_ONE_SENSOR=1`). El soporte de mux queda en el código para reactivarlo.

---

## Cómo funciona

- Driver **VL53L0X propio y autocontenido** (`vl53l0x.c`): init estándar (config 2.8 V, SPAD, tuning, calibración de referencia, presupuesto de medición) + lectura por **disparo único**.
- `distance_app` muestrea cada 5 s y decide **puerta**:
  - CERRADA si distancia `< DIST_NEAR_MM` (45 mm)
  - ABIERTA si distancia `> DIST_FAR_MM` (60 mm)
  - banda `[NEAR, FAR]` = **histéresis** (no rebota en el límite).
- **Envío solo por cambio de estado** (sin periodicidad); tope diario `DIST_MAX_TX_PER_DAY` (100).

### Parámetros (`distance_app.h`)
```c
#define DIST_USE_VL53L0X        1     // usa el driver L0X (8 bits)
#define DIST_DIRECT_ONE_SENSOR  1     // sin mux (1 sensor directo)
#define DIST_PAYLOAD_MODE       DIST_MODE_FLAGS
#define DIST_NEAR_MM            45U   // < 45 mm -> CERRADA
#define DIST_FAR_MM            60U    // > 60 mm -> ABIERTA
#define DIST_SAMPLE_MS         5000U  // muestreo
#define DIST_HEARTBEAT_MS      0U     // solo evento
```
El VL53L0X trae offset de fábrica → para mm exactos requiere calibración; para umbral/puerta no importa. **Ajusta NEAR/FAR según lo que veas en el log** de tu montaje.

---

## Payload Sigfox

Modo FLAGS → **1 byte**: `bit0` = sensor A cerca/CERRADA, `bit1` = sensor B.
(`0x00` abierta, `0x01` cerrada.)

---

## Integración y compilación

1. Copiar de `src/`: `app_features.h`, `i2c.h` → `Core/Inc`; `i2c.c` → `Core/Src`; `distance_app.*`, `vl53l0x.*`, `vl53l1_platform.*` → `Sigfox/App`.
2. Aplicar los `.patch` (`sgfx_app.c`, `utilities_def.h`, `stm32wlxx_hal_conf.h`, `.project`).
3. En `app_features.h`: `USE_DISTANCE_APP=1` (y las demás en 0).
4. Cerrar/reabrir el proyecto → Clean → Build → flash.

---

## Notas / aprendizajes

- El chip es **VL53L0X (8 bits)**, no L1X (leerlo en 16 bits daba `0x0F01`).
- **I2C no sobrevive STOP2** → `distance_app` bloquea el modo STOP mientras corre (`UTIL_LPM_SetStopMode`).
- Conectar **XSHUT a 3.3V** o el sensor no arranca.

---
*0G IoT Solutions (previamente WND México) — https://0giotsolutions.com/*
