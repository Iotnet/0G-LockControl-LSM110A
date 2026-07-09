# Programa Sensor de Flujo — YF-S201 (NUCLEO-WL55JC → Sigfox)

Añade al firmware `Sigfox_AT_Slave` V1.5.0 la medición de un **sensor de flujo de efecto Hall (YF-S201, 1-30 L/min)** como **totalizador de volumen**, con envío Sigfox del total acumulado. Pensado para **caracterización** (log por Vcom) y para reporte a backend. Mantiene en paralelo la interfaz AT.

Backend Sigfox de **0G IoT Solutions** (previamente WND México) — https://0giotsolutions.com/

---

## Contenido de la carpeta

```
Programa_Sensor_Flujo/
├── README.md
└── src/
    ├── app_features.h        Selector de apps (feature flags)          [NUEVO -> Core/Inc]
    ├── flow_app.h / .c       Lógica del sensor de flujo                [NUEVO -> Sigfox/App]
    ├── sgfx_app.c.patch      Llamada Flow_Init() en Sigfox_Init()      [MOD]
    ├── utilities_def.h.patch ID de tarea CFG_SEQ_Task_FlowTx           [MOD]
    └── dot_project.patch     Recurso enlazado (flow_app.c)             [MOD]
```

---

## Hardware / cableado

El YF-S201 tiene una turbina cuyo Hall genera un **tren de pulsos** cuya frecuencia es proporcional al caudal.

| Cable sensor | Pin Nucleo | Nota |
|--------------|-----------|------|
| Señal (amarillo) | **PB8 / D5** | open-collector → pull-up **10k a 3.3V** |
| VCC (rojo) | **5 V** | el sensor requiere 5 V |
| GND (negro) | GND | |
| Botón externo | **PB5 / D4** | pull-up + cierre a GND (uplink manual) |

---

## Cómo funciona

- Cuenta pulsos por **interrupción (EXTI)**; cada 1 s calcula **frecuencia (Hz) → caudal (L/min)** y acumula **volumen total (totalizador)**.
- Relación típica YF-S201: `F(Hz) = 7.5 × Q(L/min)` ≈ **450 pulsos/litro** (verificar en caracterización).
- **Envío (totalizador):**
  - Automático: un uplink **cada `FLOW_UPLINK_LITERS` (10 L)** acumulados.
  - Manual: **botón** → envía el total actual sin reiniciar (con `FLOW_BTN_TX=1`).
  - El total es **acumulativo** y no se reinicia → robusto ante mensajes perdidos.

### Parámetros (`flow_app.h`)
```c
#define FLOW_PULSES_PER_LITER 450U   // K típico. AJUSTAR por caracterización con agua
#define FLOW_WINDOW_MS        1000U
#define FLOW_UPLINK_LITERS    10U    // uplink cada 10 L
#define FLOW_TX_ENABLE        1
#define FLOW_BTN_TX           0/1    // 1 = boton fuerza uplink del total actual
```

### Caracterización (con agua)
Pasar volúmenes conocidos, leer `pulsos`, calcular `K = pulsos / litro`, ajustar `FLOW_PULSES_PER_LITER` y verificar linealidad en 1-30 L/min.

Salida por Vcom (115200):
```
[FLOW] f=84 Hz  Q=11200 mL/min (11.20 L/min)  Vol=20177 mL  pulsos=9080  btn=0
[FLOW] >> UPLINK (10 L) total=20177 mL (20.177 L)  caudal=11200 mL/min
```

---

## Payload Sigfox (6 de 12 bytes)

```
byte 0..3 : volumen total acumulado en mL (uint32, big-endian)
byte 4..5 : caudal instantáneo en mL/min  (uint16, big-endian)
```
Decodificación backend: `litros = bytes[0..3] / 1000`, `L/min = bytes[4..5] / 1000`.
**Ejemplo:** `000027626ef0` → `0x2762 = 10082 mL = 10.082 L` y `0x6EF0 = 28400 mL/min = 28.4 L/min`.

> Sigfox: máx 12 bytes, ~140 msg/día → con 10 L/uplink cubres ~1400 L/día. Sube `FLOW_UPLINK_LITERS` para caudales altos. Quedan 6 bytes libres (batería, flags, contador de resets).

---

## Integración y compilación

1. Copiar de `src/`: `app_features.h` → `Core/Inc`; `flow_app.*` → `Sigfox/App`.
2. Aplicar los `.patch` (`sgfx_app.c`, `utilities_def.h`, `.project`).
3. En `app_features.h`: `USE_FLOW_APP=1` (y las demás en 0, en especial `USE_BUTTONS_APP=0`, ver nota EXTI).
4. Cerrar/reabrir el proyecto → Clean → Build → flash.

---

## Notas / aprendizajes

- `flow_app.c` define su **propio `EXTI9_5_IRQHandler` + `HAL_GPIO_EXTI_Callback`** (pulsos + botón) → **no activar junto con `buttons_app`** (colisión de handlers).
- **Brown-out por energía:** el pico del TX Sigfox (o un corto por mal cableado del botón a una salida de voltaje) hunde el VDD → reset. Mitigación: fuente sólida + **capacitores de bulk (100–470 µF + 100 nF)** en 3.3V/5V y GND firme.
- El total está en **RAM** → se pierde en un reset. Pendiente: **persistencia en EEPROM** para un totalizador real.

---
*0G IoT Solutions (previamente WND México) — https://0giotsolutions.com/*
