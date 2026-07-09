# Programa Multisensor — Flujo · Hall · Distancia (NUCLEO-WL55JC → Sigfox)

Documentación técnica del código que agrega **cuatro aplicaciones de usuario** al firmware `Sigfox_AT_Slave` V1.5.0 de la **NUCLEO-WL55JC**, manteniendo en paralelo la interfaz AT y el stack Sigfox originales. Todas se seleccionan por *feature flags* (sin borrar código entre versiones) y comparten el mismo transmisor de radio.

Aplicación **activa hoy: sensor de FLUJO (YF-S201)** como totalizador de volumen — es el bloque que se estaba caracterizando. Las demás quedan integradas y listas para activarse por flag.

Operado con el backend Sigfox de **0G IoT Solutions** (previamente WND México) — https://0giotsolutions.com/

---

## Contenido

- [Aplicaciones incluidas](#aplicaciones-incluidas)
- [Contenido de esta carpeta](#contenido-de-esta-carpeta)
- [Arquitectura general](#arquitectura-general)
- [Mapa de pines](#mapa-de-pines)
- [App de flujo (activa)](#app-de-flujo-activa)
- [App de distancia / puerta (VL53L0X)](#app-de-distancia--puerta-vl53l0x)
- [App Hall magnético + botón](#app-hall-magnético--botón)
- [App de 3 botones](#app-de-3-botones)
- [Formato de payload Sigfox](#formato-de-payload-sigfox)
- [Cómo integrar y compilar](#cómo-integrar-y-compilar)
- [Bitácora técnica / aprendizajes](#bitácora-técnica--aprendizajes)
- [Pendientes](#pendientes)

---

## Aplicaciones incluidas

| App | Módulo | Sensor / función | Flag | Estado |
|-----|--------|------------------|------|--------|
| Botones | `buttons_app` | Push B1/B2/B3 de la placa → TX | `USE_BUTTONS_APP` | 0 |
| Distancia / Puerta | `distance_app` + `vl53l0x` | VL53L0X (ToF), abierto/cerrado con histéresis | `USE_DISTANCE_APP` | 0 |
| Hall magnético | `hall_app` | Sensor Hall magnético + botón (puerta) | `USE_HALL_APP` | 0 |
| **Flujo** | `flow_app` | **YF-S201 (1-30 L/min), totalizador** | `USE_FLOW_APP` | **1** |

Se activa/desactiva cada una en `app_features.h` (ver más abajo). Los flags usan `#ifndef`, así que también pueden fijarse por `-D` en una *Build Configuration* de STM32CubeIDE.

---

## Contenido de esta carpeta

```
Programa_Sensores_Flujo_Hall_Distancia/
├── README.md                     (este documento)
└── src/
    ├── app_features.h            Selector de aplicaciones (feature flags)   [NUEVO -> Core/Inc]
    ├── i2c.h  / i2c.c            Periférico I2C2 para VL53 / mux            [NUEVO -> Core/Inc, Core/Src]
    ├── distance_app.h / .c       App distancia / puerta (VL53)             [NUEVO -> Sigfox/App]
    ├── vl53l0x.h / .c            Driver propio VL53L0X (8 bits)            [NUEVO -> Sigfox/App]
    ├── vl53l1_platform.h / .c    Shim ULD VL53L1X (no usado hoy)           [NUEVO -> Sigfox/App]
    ├── hall_app.h / .c           App Hall magnético + botón                [NUEVO -> Sigfox/App]
    ├── flow_app.h / .c           App sensor de flujo (ACTIVA)              [NUEVO -> Sigfox/App]
    ├── sgfx_app.c.patch          Llamadas *_Init() en Sigfox_Init()        [MOD]
    ├── utilities_def.h.patch     IDs de tarea del sequencer                [MOD]
    ├── stm32wlxx_it.c.patch      EXTI de botones guardado por flag         [MOD]
    ├── stm32wlxx_hal_conf.h.patch Habilita HAL_I2C_MODULE_ENABLED          [MOD]
    └── dot_project.patch         Recursos enlazados (.c nuevos + driver I2C)[MOD]
```

Los `.h/.c` son **archivos nuevos completos**; los `.patch` describen los cambios a los archivos **existentes** del proyecto (siempre en bloques `USER CODE`).

---

## Arquitectura general

Patrón oficial del SDK **STM32CubeWL** con **stm32_seq** (sequencer cooperativo, bajo consumo):

```
Sigfox_Init()  ──►  <Modulo>_Init()   (bajo #if USE_*_APP)
                        │
                        ├─ registra tarea:  UTIL_SEQ_RegTask(CFG_SEQ_Task_*, ...)
                        ├─ arranca timer:   UTIL_TIMER (periódico)  o  configura EXTI
                        └─ ...
Evento (timer / EXTI / pulso)  ──►  UTIL_SEQ_SetTask(...)  ──►  <Modulo>_Task()
                                                                    │
                                                                    └─ SIGFOX_API_send_frame()  (TX compartido)
```

- Un solo camino de radio (`SIGFOX_API_send_frame`), sequencer run-to-completion → sin reentrancia.
- AT parser (Vcom) intacto y siempre activo.
- Los módulos desactivados compilan vacíos (su `#if` los envuelve).

---

## Mapa de pines

| Pin | Uso | App |
|-----|-----|-----|
| PA0 / PA1 / PC6 | Botones B1 / B2 / B3 (placa) | buttons_app |
| PA2 / PA3 | LPUART1 (Vcom / AT) | base |
| PB15 / PB9 / PB11 | LED1 / LED2 / LED3 | base |
| PB12 / PB13 | PROB1 / PROB2 | base |
| PA12 / PA11 | I2C2 SCL / SDA (Arduino D15 / D14) | distance_app |
| PB8 (D5) | Hall / señal de flujo | hall_app / flow_app |
| PB5 (D4) | Botón externo | hall_app / flow_app |

> **Pines problemáticos en esta placa:** **PB7 (D0)** está cargado por el ST-LINK (no llega a nivel alto válido en open-drain → por eso el I2C se movió a PA11/PA12). **PB5 (D4)** debe cablearse con cuidado: un mal cableado a una salida de voltaje provoca corto → *brown-out* → reinicio.

---

## App de flujo (activa)

`flow_app.c/.h` — sensor de flujo de efecto Hall tipo **YF-S201** (turbina; frecuencia de pulsos ∝ caudal).

**Cableado**

| Cable | Pin | Nota |
|-------|-----|------|
| Señal (amarillo) | **PB8 / D5** | open-collector → pull-up 10k **a 3.3 V** |
| VCC (rojo) | **5 V** | el sensor requiere 5 V |
| GND (negro) | GND | |
| Botón externo | **PB5 / D4** | pull-up + cierre a GND |

**Cómo funciona:** cuenta pulsos por **EXTI**; cada 1 s calcula frecuencia (Hz) → caudal (L/min) y acumula **volumen total (totalizador)**.

**Parámetros (`flow_app.h`)**

```c
#define FLOW_PULSES_PER_LITER 450U   // K típico YF-S201 (F=7.5*Q). AJUSTAR por caracterización
#define FLOW_WINDOW_MS        1000U
#define FLOW_UPLINK_LITERS    10U    // totalizador: uplink cada 10 L
#define FLOW_TX_ENABLE        1
#define FLOW_BTN_TX           0      // 1 = el botón fuerza un uplink del total actual
```

**Envío (totalizador):** uplink automático **cada 10 L** acumulados, y uplink **manual con el botón** (envía el total actual, sin reiniciar el contador, con `FLOW_BTN_TX=1`). El total es acumulativo y no se reinicia → robusto ante mensajes perdidos.

**Caracterización (con agua):** pasar volúmenes conocidos, leer `pulsos`, calcular `K = pulsos / litro`, ajustar `FLOW_PULSES_PER_LITER` y verificar linealidad en 1-30 L/min.

Salida por Vcom (115200):
```
[FLOW] f=84 Hz  Q=11200 mL/min (11.20 L/min)  Vol=20177 mL  pulsos=9080  btn=0  PB5=1
[FLOW] >> UPLINK (10 L) total=20177 mL (20.177 L)  caudal=11200 mL/min
```

---

## App de distancia / puerta (VL53L0X)

`distance_app.c/.h` + `vl53l0x.c/.h`. Sensor ToF **VL53L0X** (registros de 8 bits, WHO_AM_I `0xC0=0xEE`) por I2C2. Driver **propio y autocontenido** (init estándar ST/Pololu + lectura por disparo único).

- `DIST_USE_VL53L0X=1`, `DIST_DIRECT_ONE_SENSOR=1` (el mux **PCA9548A resultó dañado** → un sensor directo).
- Modo puerta con histéresis: CERRADA si distancia < `DIST_NEAR_MM` (45 mm), ABIERTA si > `DIST_FAR_MM` (60 mm); muestreo 5 s, **envío solo por cambio**; tope diario 100.
- El VL53L0X trae offset de fábrica; para mm exactos requiere calibración (para umbral/puerta no importa).
- `vl53l1_platform.c` es el shim para el ULD del VL53**L1X** (por si el sensor fuera L1X); no se usa con el L0X.

**Payload (modo FLAGS):** 1 byte → `bit0`=sensor A cerca/cerrada, `bit1`=sensor B.

---

## App Hall magnético + botón

`hall_app.c/.h`. Sensor Hall de conmutación (puerta magnética) + botón externo, ambos con **filtro RC (10k + 1nF)**. Sondeo 200 ms con debounce, **envío solo por cambio**. Pines HALL→PB8 (D5), botón→PB5 (D4), activos en bajo.

**Payload:** 1 byte → `bit0`=Hall (imán/cerrada), `bit1`=botón.

---

## App de 3 botones

`buttons_app.c` (envuelto en `#if USE_BUTTONS_APP`). Envío Sigfox al presionar B1/B2/B3 de la placa (demo). Ver también la carpeta `Programa_3_botones` del repo.

---

## Formato de payload Sigfox

| App | Bytes | Formato |
|-----|-------|---------|
| flow_app | 6 | `[vol_mL uint32 BE][caudal mL/min uint16 BE]` |
| distance_app (FLAGS) | 1 | `bit0`=A cerca, `bit1`=B |
| hall_app | 1 | `bit0`=Hall, `bit1`=botón |

**Ejemplo flujo:** `000027626ef0` → `0x00002762 = 10082 mL = 10.082 L` y `0x6EF0 = 28400 mL/min = 28.4 L/min`.
Decodificación backend: `litros = bytes[0..3] / 1000`, `L/min = bytes[4..5] / 1000`.

> Sigfox: máx **12 bytes** y ~**140 mensajes/día**. El totalizador con 10 L/uplink cubre ~1400 L/día antes del límite (subir umbral para caudales altos). Quedan **6 bytes libres** en el payload de flujo (batería, flags, contador de resets, etc.).

---

## Cómo integrar y compilar

1. Copiar los archivos nuevos de `src/` al proyecto `Sigfox_AT_Slave`:
   - `app_features.h`, `i2c.h` → `Core/Inc/`
   - `i2c.c` → `Core/Src/`
   - `distance_app.*`, `vl53l0x.*`, `vl53l1_platform.*`, `hall_app.*`, `flow_app.*` → `Sigfox/App/`
2. Aplicar los `.patch` a los archivos existentes (`sgfx_app.c`, `utilities_def.h`, `stm32wlxx_it.c`, `stm32wlxx_hal_conf.h`, `.project`).
3. En `app_features.h`, dejar activa la app deseada (`USE_FLOW_APP=1`, resto en 0).
4. **Cerrar y reabrir el proyecto** en STM32CubeIDE (para que tome los `<linkedResources>` nuevos) → **Project → Clean → Build** → flashear.
5. Ver salida por Vcom (115200).

> Este proyecto enlaza cada fuente por separado en `.project`; todo `.c` nuevo debe agregarse ahí (ver `dot_project.patch`) o no se compila.

---

## Bitácora técnica / aprendizajes

- **El sensor de flujo es VL53L0X, no L1X** (registros de 8 bits; leerlo en 16 bits daba basura `0x0F01`).
- **PCA9548A (mux) dañado:** arrastraba SDA → se usó un sensor directo.
- **I2C no sobrevive STOP2:** `distance_app` bloquea el modo STOP mientras está activa (`UTIL_LPM_SetStopMode`). `flow_app`/`hall_app` solo usan GPIO y sí toleran STOP2.
- **Brown-out por energía:** el pico de corriente del TX Sigfox (o un corto por mal cableado del botón) hunde el VDD → reset. Mitigación: fuente sólida + **capacitores de bulk (100–470 µF + 100 nF)** en 3.3 V y 5 V, y GND firme.
- **Filtro RC (10k + 1nF):** filtra ruido/EMI de alta frecuencia y limpia flancos (evita cuentas falsas). `fc ≈ 16 kHz`, muy por encima de los pulsos (≤ ~225 Hz) → no distorsiona la señal. El rebote mecánico (ms) lo maneja el software.
- **Pines:** evitar PB7 (D0, cargado por ST-LINK). El I2C quedó en PA11/PA12 (D14/D15).
- **Colisión de EXTI:** `flow_app` y `buttons_app` definen ambos `EXTI9_5_IRQHandler` → no activarlas juntas.

---

## Pendientes

1. **Caracterización del sensor de flujo con agua** (K-factor y linealidad) → ajustar `FLOW_PULSES_PER_LITER`.
2. **Persistencia en EEPROM** del volumen total (que no se pierda ante reinicios/cortes).
3. **Endurecimiento de energía** (bulk caps, fuente estable) para eliminar los brown-out.
4. **Limpieza de logs de diagnóstico** (`PB5=`, `BOTON presionado`, escáner I2C, pin-test) para producción.
5. **Reponer el mux PCA9548A** para 2 sensores de distancia simultáneos.
6. **Campos extra de payload** (6 bytes libres en flujo).

---

*Documento generado para 0G IoT Solutions (previamente WND México) — https://0giotsolutions.com/*
