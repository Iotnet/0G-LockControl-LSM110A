# Programa Sensor Hall Magnético + Botón (NUCLEO-WL55JC → Sigfox)

Añade al firmware `Sigfox_AT_Slave` V1.5.0 la lectura de un **sensor Hall magnético** (tipo puerta: imán presente / ausente) y un **botón externo**, ambos con **filtro RC por hardware (10k + 1nF)**, con envío Sigfox **solo por evento**. Mantiene en paralelo la interfaz AT.

Backend Sigfox de **0G IoT Solutions** (previamente WND México) — https://0giotsolutions.com/

---

## Contenido de la carpeta

```
Programa_Sensor_Hall_Magnetico/
├── README.md
└── src/
    ├── app_features.h        Selector de apps (feature flags)          [NUEVO -> Core/Inc]
    ├── hall_app.h / .c       Lógica Hall + botón                       [NUEVO -> Sigfox/App]
    ├── sgfx_app.c.patch      Llamada Hall_Init() en Sigfox_Init()      [MOD]
    ├── utilities_def.h.patch ID de tarea CFG_SEQ_Task_HallTx           [MOD]
    └── dot_project.patch     Recurso enlazado (hall_app.c)             [MOD]
```

---

## Hardware / cableado

Dos entradas digitales, cada una con filtro RC (10k a 3.3V + 1nF a GND; el sensor/botón cierra a GND → reposo ALTO, activo en BAJO):

| Entrada | Pin Nucleo | Header |
|---------|-----------|--------|
| Sensor Hall (salida) | **PB8** | Arduino D5 |
| Botón externo | **PB5** | Arduino D4 |

> El filtro RC (`fc ≈ 16 kHz`) filtra ruido/EMI y limpia flancos; el rebote mecánico lo termina el debounce por software.

---

## Cómo funciona

- Sondeo cada **200 ms** (`hall_app`), con debounce por software (2 lecturas coincidentes).
- **Envío solo por cambio de estado** (sin periodicidad); tope diario `HALL_MAX_TX_PER_DAY` (100).
- Activo en bajo por defecto (`HALL_ACTIVE_LOW=1`).

### Parámetros (`hall_app.h`)
```c
#define HALL_Pin            GPIO_PIN_8   // PB8 / D5
#define BTN_EXT_Pin         GPIO_PIN_5   // PB5 / D4
#define HALL_SAMPLE_MS      200U
#define HALL_MAX_TX_PER_DAY 100U
```

---

## Payload Sigfox

**1 byte**: `bit0` = Hall (imán presente / puerta CERRADA), `bit1` = botón presionado.
(`0x00` abierta/suelto, `0x01` cerrada, `0x02` botón, `0x03` ambos.)

---

## Integración y compilación

1. Copiar de `src/`: `app_features.h` → `Core/Inc`; `hall_app.*` → `Sigfox/App`.
2. Aplicar los `.patch` (`sgfx_app.c`, `utilities_def.h`, `.project`).
3. En `app_features.h`: `USE_HALL_APP=1` (y las demás en 0).
4. Cerrar/reabrir el proyecto → Clean → Build → flash.

---

## Notas

- Usa **sondeo** (no EXTI) → sin conflicto con otros handlers y compatible con bajo consumo (solo GPIO).
- **PB5 (D4):** cablear con cuidado; un mal cableado a una salida de voltaje provoca corto → *brown-out* → reinicio.

---
*0G IoT Solutions (previamente WND México) — https://0giotsolutions.com/*
