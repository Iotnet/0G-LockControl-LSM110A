# 0G LockControl LSM110A — Handoff Firmware (Milestone M2)

**Empresa:** 0G IoT Solutions (previamente WND México)
**Web:** https://0giotsolutions.com/
**Repo:** [github.com/jdiaznxt/0G-LockControl-LSM110A](https://github.com/jdiaznxt/0G-LockControl-LSM110A)
**Autor:** Yahir Flores — yflores@iotnet.mx
**Co-desarrollo:** José Francisco Díaz (Franco) — jdiaz@iotnet.mx
**Versión:** 1.0 — Junio 2026

---

## Índice

1. [Contexto del proyecto](#1-contexto-del-proyecto)
2. [Setup del entorno](#2-setup-del-entorno)
3. [Estado actual del firmware](#3-estado-actual-del-firmware)
4. [Cambios pendientes en main.c (SDK)](#4-cambios-pendientes-en-mainc-sdk)
5. [Backlog M2 dividido Yahir / Franco](#5-backlog-m2-dividido-yahir--franco)
6. [Convenciones de código](#6-convenciones-de-código)
7. [Prompt de arranque para Claude Code](#7-prompt-de-arranque-para-claude-code)
8. [Referencias rápidas](#8-referencias-rápidas)
9. [Mapa completo de archivos (rutas absolutas)](#9-mapa-completo-de-archivos-rutas-absolutas)

---

## 1. Contexto del proyecto

**Producto:** dispositivo de detección de apertura de puerta con transmisión Sigfox 0G RC2 México. Basado en el módulo **LSM110A** (STM32WL con firmware API integrado). Batería CR2450, low-power modo Stop2, wake-up por acelerómetro LIS2DW12 (I2C) y sensor magnético reed/DRV5032 (GPIO EXTI).

**Milestone actual:** M2 Firmware (semanas 2-5 del roadmap 12-week MVP).

**Estado M0/M1:**
- M0 Setup — completado: repo, spec v1.0, SDK clonado y compilado, TX Sigfox validado en Nucleo con backend 0G.
- M1 Esquemático — responsabilidad HW (Lead), avanza en paralelo.

**Roadmap posterior:**
| Milestone | Alcance | Semanas |
|---|---|---|
| M2 | FW Nucleo (accel + reed + Sigfox) | 2-5 |
| M3 | PCB layout + gerbers | 5-7 |
| M4 | Low power validado en Nucleo | 7-8 |
| M5 | PCB fabricada y ensamblada | 8-11 |
| M6 | Integración FW en PCB custom | 11-12 |
| M7 | Prueba de campo | 12 |

---

## 2. Setup del entorno

### 2.1 Repositorios

| Repo | Ruta local | Propósito |
|---|---|---|
| `0G-LockControl-LSM110A` | `~/GitHub/0G-LockControl-LSM110A` | Repo principal del producto (drivers, docs, spec) |
| `Support-SJI/LSM110A` | `~/GitHub/LSM110A` | SDK oficial del fabricante SJI (Nucleo + LSM110A) |

**Importante:** el SDK vive **fuera** del repo principal. Los drivers custom se enlazan vía linked resources (no se copian) para mantener una sola fuente de verdad versionada.

### 2.2 Proyecto STM32CubeIDE

**Ruta a importar:**
```
~/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE
```

**Nombre del proyecto:** `LSM1x0A_SDK_LoRaWAN_Sigfox`

**Toolchain:** GNU Tools for STM32 13.3.rel1 (viene con CubeIDE)

### 2.3 Reloj y periféricos

- **SYSCLK:** HSI a **16 MHz**
- **PCLK1:** 16 MHz (I2C1 usa PCLK1)
- **I2C1:** 100 kHz Standard Mode — `Timing = 0x10707DBC` (valor oficial ST para 16 MHz)
- **GPIO relevantes:**
  - `PA0` — INT1 del LIS2DW12 (EXTI0, rising, pull-down)
  - `PA1` — reed switch / DRV5032 (EXTI1, both edges, pull-up)
  - `PA9` — I2C1_SCL (AF4, open-drain, sin pull interno — usar pull-up externo 4.7 kΩ)
  - `PA10` — I2C1_SDA (AF4, open-drain, sin pull interno)

### 2.4 Import de drivers al proyecto CubeIDE

> **✅ Ya aplicado (1-jul-2026):** los linked resources de `drivers/` y `app/`
> (incluye `app_sensors.c/.h` y `payload_codec.c/.h`) ya están en el
> `.project`/`.cproject`, junto con los include paths físicos y los sources HAL
> de I2C/IWDG. **No repetir estos pasos.** Detalle en `docs/sdk-patches.md`.
> Los pasos siguientes quedan como referencia para un re-clone del SDK.

Los drivers custom viven en `~/GitHub/0G-LockControl-LSM110A/Firmware/drivers/` y se importan al proyecto como **linked resources**:

1. Click derecho sobre `Application/User` → `New` → `Folder` → nombre: `drivers`.
2. Click derecho sobre la nueva carpeta → `Import` → `General` → `File System`.
3. `From directory:` `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers`.
4. Seleccionar los 6 archivos: `bsp_lockcontrol.c/.h`, `lis2dw12.c/.h`, `reed_switch.c/.h`. **NO** importar `.gitkeep` ni `README.md`.
5. Click `Advanced` → marcar:
   - ✅ **Create links in workspace**
   - ✅ **Create virtual folders**
   - ✅ **Create link locations relative to:** `PROJECT_LOC`
6. `Finish`.

**Include path:**
- `Project` → `Properties` → `C/C++ Build` → `Settings` → `MCU GCC Compiler` → `Include paths` → añadir `../Application/User/drivers`.

---

## 3. Estado actual del firmware

### 3.1 Drivers ya escritos y validados en build (0 errores)

**Ubicación:** `Firmware/drivers/` en el repo principal.

| Archivo | Propósito |
|---|---|
| `bsp_lockcontrol.c/.h` | BSP: init I2C1 + GPIO EXTI0/1 + IRQ handlers |
| `lis2dw12.c/.h` | Driver acelerómetro I2C (WHO_AM_I, wake-up config, event API) |
| `reed_switch.c/.h` | Driver sensor magnético con debounce 50 ms |
| `README.md` | Guía de integración y uso |

### 3.2 Filosofía de diseño

- **ISR minimalista, main loop procesa.** El callback EXTI solo levanta un flag `volatile bool event_pending`. La lectura de registros por I2C (LIS2DW12) o del pin (reed) se hace en el main loop / task.
- **Handles inyectados.** Los drivers reciben `I2C_HandleTypeDef*` y `GPIO_TypeDef*` por parámetro. Ningún acoplamiento a hardware específico.
- **Sin FreeRTOS ni RTOS.** Compatibles con super-loop simple y con el scheduler cooperativo `stm32_seq.h` del SDK.

### 3.3 Build actual

Último build (1-jul-2026, headless CubeIDE): `0 errors, 6 warnings, ~19 s` —
**primer build que compila y linkea realmente los drivers custom + app_sensors
+ payload codec** (los builds previos no incluían los linked resources; ver
`docs/sdk-patches.md`, hallazgo 1).

Los 6 warnings son pre-existentes del SDK (3× `usart.h:32`, `lora_at.c:2970`,
`sgfx_app_api.c:175`, linker RWX) — no de nuestro código. Ignorables.

---

## 4. Cambios pendientes en main.c (SDK)

> **✅ Ya aplicado (1-jul-2026)** con un diseño mejorado: `g_accel`/`g_reed` no
> viven en `main.c` sino como estáticos de `Firmware/app/app_sensors.c` (repo
> principal, versionado). `main.c` solo llama `BSP_LockControl_Init()` y
> despacha las EXTI a `app_sensors_notify_*_isr()`, que agendan tasks del
> sequencer (Y3). El detalle exacto de cada edit al SDK está en
> **`docs/sdk-patches.md`**. Las subsecciones 4.1-4.5 quedan como referencia
> histórica del plan original.

**Archivo a editar:** `Core/Src/main.c` (dentro del proyecto CubeIDE, es decir en `~/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Core/Src/main.c`).

**Advertencia:** este archivo vive en el SDK clonado, no en el repo principal. Los cambios se hacen directo. Documentar cada edit para reproducibilidad.

### 4.1 Cambio 1 — Includes

**Después de la línea `#include "radio_board_if.h"` (línea 26):**

```c
/* USER CODE BEGIN Includes */
#include "bsp_lockcontrol.h"
#include "lis2dw12.h"
#include "reed_switch.h"
/* USER CODE END Includes */
```

### 4.2 Cambio 2 — Variables globales

**Después de `uint32_t active_app;` (línea 30):**

```c
/* USER CODE BEGIN PV */
lis2dw12_t    g_accel;
reed_switch_t g_reed;
/* USER CODE END PV */
```

### 4.3 Cambio 3 — Init de BSP y drivers

**Después de `SystemClock_Config();` (línea 44) y antes de `active_app = SystemApp_Init();`:**

```c
  /* USER CODE BEGIN SysInit */
  BSP_LockControl_Init();

  lis2dw12_init_handle(&g_accel, &hi2c1);
  reed_switch_init(&g_reed, REED_GPIO_PORT, REED_PIN, 50);
  /* USER CODE END SysInit */
```

### 4.4 Cambio 4 — HAL_GPIO_EXTI_Callback

**Reemplazar el bloque vacío `USER CODE BEGIN 4` / `USER CODE END 4` (líneas 112-114) por:**

```c
/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ACCEL_INT1_PIN) {
        lis2dw12_on_interrupt(&g_accel);
    }
    else if (GPIO_Pin == REED_PIN) {
        reed_switch_on_interrupt(&g_reed);
    }
}
/* USER CODE END 4 */
```

### 4.5 Verificación

Después de los 4 cambios: `Cmd+B`.

- **Esperado:** 0 errores, warnings pueden subir a 8 por `g_accel/g_reed defined but not used` (esperado hasta que se enganche el polling en `app_sigfox.c`).
- **Si falla:** verificar que el include path incluye `../Application/User/drivers`.

---

## 5. Backlog M2 dividido Yahir / Franco

Objetivo: cerrar M2 en las semanas restantes. División basada en fuerzas: Yahir en drivers / integración app, Franco en payload / networking / testing.

### 5.1 Yahir (drivers + wake-up completo)

| ID | Estado | Tarea | Archivo(s) | Criterio de aceptación |
|---|---|---|---|---|
| Y1 | ✅ 1-jul | Config wake-up LIS2DW12 en boot | `Firmware/app/app_sensors.c` | `lis2dw12_config_wakeup(200)` tras WHO_AM_I (con 3 reintentos). Log UART: "Accel OK, WHO_AM_I=0x44". |
| Y2 | ✅ 1-jul | Polling en main loop del Sigfox | `app_sensors.c` + `app_sigfox.c` | Tasks del sequencer procesan y disparan TX real (`SIGFOX_API_send_frame`, cooldown 60 s); `app_sensors_poll()` como red de seguridad en `MX_Sigfox_Process`. |
| Y3 | ✅ 1-jul | Enganche al scheduler `stm32_seq.h` | `utilities_def.h`, `main.c`, `app_sensors.c` | `CFG_SEQ_Task_ACCEL_EVENT`/`REED_EVENT` definidas, registradas con `UTIL_SEQ_RegTask`, disparadas con `UTIL_SEQ_SetTask` desde el callback EXTI. |
| Y4 | ⬜ | Test bench en Nucleo | — | Golpear la mesa → log `EVT accel` + TX. Alejar imán → log `EVT reed` + TX. Payload visible en backend. Documentar con foto/GIF en `docs/Images/`. |
| Y5 | ⚠️ ver nota | IWDG watchdog | `bsp_lockcontrol.c` + `app_sigfox.c` | Implementado con `LOCKCONTROL_IWDG_ENABLE` (default 0) y timeout 16 s. **4 s era inviable:** el TX Sigfox RC2 bloquea 7-9 s y el build actual duerme en Stop2 (IWDG resetearía en idle). Habilitar solo en bench con `LOW_POWER_DISABLE=1`; solución definitiva en M4 (option byte `IWDG_STOP=frozen`). Ver `docs/sdk-patches.md`, hallazgo 4. |

### 5.2 Franco (payload + Sigfox + testing)

| ID | Tarea | Archivo(s) | Criterio de aceptación |
|---|---|---|---|
| F1 | Serializer payload 12 bytes | `Firmware/app/payload.c/.h` (nuevo) | Struct `lockcontrol_payload_t` según spec sec 4.4. Función `payload_pack(*payload, uint8_t buf[12])` en big-endian. Unit test con vector conocido. |
| F2 | Parser payload 12 bytes | mismo archivo | Función `payload_unpack(uint8_t buf[12], *payload)`. Round-trip test: pack → unpack devuelve struct original. |
| F3 | Integración TX Sigfox | `app_sigfox.c` | Función `sigfox_send_alarm(source, magnitude, axis, reed_state)`. Usa `SGFX_APP_ExecUplink(...)` del SDK. Cooldown 60 s entre TX. |
| F4 | Heartbeat 24h | `app_sensors.c` | RTC alarm cada 24 h. Payload con `bateria%`, `temp`, `conteo_eventos`, `reed_state actual`. |
| F5 | Validación end-to-end backend Sigfox | — | Payload recibido en backend.sigfox.com. Decodificado correctamente en callback. Screenshot en `docs/Images/`. |

### 5.3 Bloqueos y dependencias

- **Y2 bloquea F3** — Franco necesita que Yahir exponga la API `sensors_get_last_event()` para armar payload.
- **F1 bloquea Y2** — Yahir necesita el serializer para armar el payload dentro del polling.
- **Solución:** F1 primero (2-3 horas), luego Y2 en paralelo con F3.

### 5.4 Fuera de M2 (para M4 y adelante)

- Stop2 low power validado con multímetro.
- FUOTA — no aplica al MVP (Sigfox uplink-first).
- Callback URL en backend — es rol **Persona (App)** en Notion, no FW.

---

## 6. Convenciones de código

### 6.1 Naming

- **Funciones públicas del driver:** `<driver>_<verb>()`. Ej: `lis2dw12_read_reg`, `reed_switch_process_event`.
- **Prefijos globales:** `g_` para singletons (`g_accel`, `g_reed`).
- **Macros:** `SNAKE_UPPER`. `#define ACCEL_INT1_PIN GPIO_PIN_0`.
- **Enums:** `PascalCase` para el tipo, `SNAKE_UPPER` para valores. Ej: `reed_state_t { REED_CLOSED, REED_OPEN }`.

### 6.2 Return types

- Drivers devuelven un enum de status (`lis2dw12_status_t`). Nunca `int` o `bool` para operaciones que pueden fallar.
- Funciones que no pueden fallar devuelven `void`.

### 6.3 ISR vs main loop

- **En ISR** — solo escritura a variables `volatile`. Nada de I/O bloqueante (I2C, UART, SPI). Máximo 5-10 µs por llamada.
- **En main loop / task** — todo lo demás. Se puede bloquear con timeouts razonables.

### 6.4 Comentarios

- Comentarios de header estilo Doxygen (`/**  ... */`).
- En línea: castellano técnico, sin acentos (para evitar problemas de encoding en toolchains viejos).
- Explicar el **por qué**, no el **qué**.

### 6.5 Git

- Rama principal: `main`.
- Feature branches: `feature/<milestone>-<tarea>`, ej: `feature/m2-payload`.
- Commits en formato conventional: `feat(fw):`, `fix(drivers):`, `docs:`, `chore:`.

### 6.6 Empresa en documentos

Todo documento entregado en el repo debe referirse a **0G IoT Solutions**. La forma "previamente WND México" o "IOTNET México" se puede usar en contexto formal donde aporte contexto histórico. URL: https://0giotsolutions.com/.

---

## 7. Prompt de arranque para Claude Code

**Contexto que debes proporcionarle en la primera interacción:**

```
Eres asistente de firmware embebido para el proyecto 0G LockControl LSM110A
de 0G IoT Solutions (previamente WND México).

Repositorios locales:
- Repo principal: ~/GitHub/0G-LockControl-LSM110A
- SDK del fabricante: ~/GitHub/LSM110A (Support-SJI/LSM110A)

Rol asignado: firmware developer trabajando en Milestone M2. El estado actual
y las tareas asignadas están en docs/Guides/HANDOFF-FW-M2.md del repo principal.
Léelo COMPLETO antes de responder cualquier cosa.

Hardware:
- MCU: STM32WL55JC (dev en Nucleo-WL55JC, luego LSM110A en PCB custom).
- Toolchain: STM32CubeIDE 1.18+, GCC arm-none-eabi.
- SYSCLK: 16 MHz HSI. PCLK1: 16 MHz.
- I2C1 PA9/PA10 a 100 kHz (Timing 0x10707DBC).
- EXTI0 = PA0 (accel INT1). EXTI1 = PA1 (reed switch).

Convenciones ya establecidas:
- Drivers en Firmware/drivers/ del repo principal, importados al proyecto
  CubeIDE con linked resources (no copias).
- ISR-safe: solo levantar flags en interrupciones, I/O en main loop.
- Sin RTOS. Compatibles con super-loop y con stm32_seq.h del SDK.
- Retornos de driver: enums de status, no int/bool.

Estilo de respuesta:
- Español conciso, sin acentos en comentarios de código.
- Cuando propongas cambios, incluye ruta absoluta y el diff exacto.
- Cuando toques main.c del SDK, avisa que es un archivo del clone del SDK
  (no del repo principal).
- Empresa: "0G IoT Solutions" en cualquier documento generado.

Primer request del usuario:
[AQUÍ VA LA TAREA CONCRETA — por ejemplo:
 "implementa la tarea F1 del backlog: serializer payload 12 bytes"
 "engancha el driver LIS2DW12 al scheduler stm32_seq del SDK"]
```

### 7.1 Buenas prácticas al usarlo

- **No pegarle todo el repo.** Claude Code puede navegar; solo dale rutas y él lee.
- **Una tarea a la vez.** Cerrar Y1 antes de pasar a Y2 o cambiar el foco.
- **Verificar builds antes de siguiente tarea.** `arm-none-eabi-gcc` debe salir con 0 errores.
- **Commits atómicos.** No dejar que combine dos features en un commit.

---

## 8. Referencias rápidas

### 8.1 Documentación técnica

- [Datasheet STM32WL55JC](https://www.st.com/en/microcontrollers-microprocessors/stm32wl55jc.html)
- [Reference Manual STM32WL (RM0453)](https://www.st.com/resource/en/reference_manual/rm0453-stm32wl5x-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [Datasheet LIS2DW12](https://www.st.com/resource/en/datasheet/lis2dw12.pdf)
- [Application Note LIS2DW12 (AN5038)](https://www.st.com/resource/en/application_note/an5038-lis2dw12-3axis-mems-accelerometer-ultralowpower-stmicroelectronics.pdf)
- [SDK Support-SJI/LSM110A](https://github.com/Support-SJI/LSM110A)

### 8.2 Backend y herramientas

- Backend Sigfox: https://backend.sigfox.com
- Grupo del proyecto: `DHW`
- Zona: **RC2 México** — 902.1782 MHz
- Device Nucleo actual (validado el 29-mayo-2026):
  - Device ID: `033E07FC`
  - PAC (rotado): `BC07485F3FFF8B71`

### 8.3 Recursos internos

- Notion Hub: [0G LockControl — Hub del Proyecto](https://star-muskmelon-e73.notion.site/0G-LockControl-Hub-del-Proyecto-35a00f12d6428186937acde842960f08)
- Notion Task Board: 53 tareas del MVP filtrables por milestone/responsable/semana.
- Guía de flujo de trabajo diario: notion.site/Flujo-de-trabajo-diario-Guia

### 8.4 Contactos

| Rol | Persona | Email |
|---|---|---|
| FW Lead | Yahir Flores | yflores@iotnet.mx |
| HW / co-FW | José Francisco Díaz (Franco) | jdiaz@iotnet.mx |

---

---

## 9. Mapa completo de archivos (rutas absolutas)

Toda ruta es local a la Mac de Yahir. Claude Code puede leer/editar directamente.

### 9.1 Repo principal — `~/GitHub/0G-LockControl-LSM110A`

**Documentación y specs:**

| Ruta | Descripción |
|---|---|
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/README.md` | README del producto |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/docs/spec-producto.md` | Spec v1.0 |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/docs/pinout-lsm110a.md` | Pinout completo del LSM110A |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/docs/arquitectura-justificacion.md` | Justificación arquitectura |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/docs/decisiones-tecnicas.md` | Log de decisiones |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/docs/Guides/HANDOFF-FW-M2.md` | **Este documento** |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/docs/Guides/Guia_completa_NUCLEO-WL55JC_Sigfox.pdf` | Guía end-to-end del aprovisionamiento Sigfox |

**Drivers (linked resources en el proyecto CubeIDE):**

| Ruta | Descripción |
|---|---|
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/bsp_lockcontrol.h` | Header BSP |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/bsp_lockcontrol.c` | Init I2C1 + EXTI + IRQ handlers |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/lis2dw12.h` | Header driver acelerómetro |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/lis2dw12.c` | Implementación LIS2DW12 |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/reed_switch.h` | Header sensor magnético |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/reed_switch.c` | Implementación reed / DRV5032 |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/drivers/README.md` | Guía de uso de drivers |

**Carpetas destino para nuevo código (aún vacías, `.gitkeep` como placeholder):**

| Ruta | Uso previsto |
|---|---|
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/app/` | Lógica de aplicación (F1, F2, Y2) — `payload.c/.h`, `event_dispatch.c/.h`, etc. |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/config/` | Config del proyecto — `.ioc`, linker scripts, `credenciales_sigfox.h` |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/libs/` | Librerías externas si se aíslan del SDK |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/tools/` | Scripts (flash, decoders Python, etc.) |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/src/` | Reservado — actualmente vacío (se movió a app/ y drivers/) |

**Referencias históricas:**

| Ruta | Descripción |
|---|---|
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Firmware/src/main_reference_nucleo.c` | Código de referencia validado en Nucleo (Oct-Ene 2025-2026). NO es el código final; sirve como referencia. |
| `/Users/yflores/GitHub/0G-LockControl-LSM110A/Acondicionamiento del NUCLEO-WL55JC al backend de Sigfox/` | Procedimiento completo de vinculación al backend Sigfox 0G |

### 9.2 SDK del fabricante — `~/GitHub/LSM110A`

Base: `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A`

**Archivos del SDK que se editan en M2:**

| Ruta absoluta | Editar para |
|---|---|
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Core/Src/main.c` | Sección 4 de este doc — includes, globals, init BSP, HAL_GPIO_EXTI_Callback |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Core/Inc/main.h` | Añadir `extern I2C_HandleTypeDef hi2c1;` si no se resuelve por include chain |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Core/Src/stm32wlxx_it.c` | (Solo si el linker se queja de handlers duplicados — no debería) |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Sigfox/App/app_sigfox.c` | Tareas Y2, F3 — enganche del polling / envío Sigfox |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Sigfox/App/app_sigfox.h` | Declarar funciones expuestas de la app |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/Core/Src/sys_app.c` | Config de bajo nivel de la app del SDK (usualmente no se toca) |

**Archivos del SDK de solo lectura (referencia, NO editar):**

| Ruta | Descripción |
|---|---|
| `/Users/yflores/GitHub/LSM110A/Drivers/STM32WLxx_HAL_Driver/` | HAL de STM (I2C, GPIO, RCC, etc.) |
| `/Users/yflores/GitHub/LSM110A/Middlewares/Third_Party/Sigfox/` | Stack Sigfox propietario ST |
| `/Users/yflores/GitHub/LSM110A/Middlewares/Third_Party/SubGHz_Phy/` | Capa PHY de radio |
| `/Users/yflores/GitHub/LSM110A/Utilities/sequencer/stm32_seq.c` | Scheduler cooperativo (usarlo, no modificar) |
| `/Users/yflores/GitHub/LSM110A/Utilities/lpm/tiny/stm32_lpm.c` | Gestor de low power modes |

**Proyecto CubeIDE:**

| Ruta | Uso |
|---|---|
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE/.project` | Archivo del workspace CubeIDE (auto-gestionado) |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE/.cproject` | Config del compilador CDT (linked resources aparecen aquí como `<link>`) |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE/Debug/LSM1x0A_SDK_LoRaWAN_Sigfox.elf` | Output del build (regenerado en cada build) |
| `/Users/yflores/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE/Debug/LSM1x0A_SDK_LoRaWAN_Sigfox.hex` | Firmware flasheable |

### 9.3 Convención de acceso para Claude Code

- **Puede leer libremente:** todo lo listado arriba.
- **Puede editar sin preguntar:** el repo principal `0G-LockControl-LSM110A` completo.
- **Editar previo aviso:** archivos del SDK bajo `~/GitHub/LSM110A/Projects/...`. Es un clone de repo externo, así que documentar cada edit en el commit message del repo principal (via un archivo `docs/sdk-patches.md` si se acumulan varios).
- **No tocar:** todo lo que esté bajo `Drivers/`, `Middlewares/`, `Utilities/` del SDK (líneas HAL / stack Sigfox / scheduler). Si algo falta ahí, es señal de que se necesita otra estrategia (fork del SDK, submodule, etc.).

---

**Empresa operadora:** 0G IoT Solutions (previamente WND México) — https://0giotsolutions.com/

*Documento vivo — actualizar en cada cierre de milestone.*
