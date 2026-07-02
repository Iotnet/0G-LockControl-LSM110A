# Parches aplicados al SDK (Support-SJI/LSM110A)

**Empresa:** 0G IoT Solutions
**Proyecto:** 0G LockControl LSM110A — Milestone M2
**Ultima actualizacion:** 1-julio-2026

El SDK del fabricante (`~/GitHub/LSM110A`) es un clone externo sin control de
versiones nuestro. Este documento registra **cada edicion** hecha a archivos del
SDK para poder reproducirlas tras un re-clone o update del fabricante.

Base de rutas: `~/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/`

---

## Estado tras los parches (1-jul-2026)

Build headless verificado: **0 errores, 6 warnings (todos pre-existentes del
SDK), 18.7 s**. Primer build en incluir realmente los drivers custom (ver
hallazgo 1).

---

## Hallazgos que motivaron parches

1. **Los drivers custom nunca se habian compilado.** Los linked resources se
   anadieron al `.project` el 2-jun a las 16:47; el ultimo `.elf` era de las
   08:46 del mismo dia. El "build 0 errores" previo compilo el SDK *sin*
   nuestros archivos.
2. **`HAL_I2C_MODULE_ENABLED` estaba comentado** en `stm32wlxx_hal_conf.h` y
   `stm32wlxx_hal_i2c.c/_ex.c` no estaban en el proyecto. El primer build real
   de `bsp_lockcontrol.c`/`lis2dw12.c` habria fallado (no existe
   `I2C_HandleTypeDef`).
3. **Los include paths `../Application/User/...` apuntan a carpetas virtuales
   de Eclipse** que no existen en disco: GCC nunca los resuelve. Los sources
   linkeados si compilan (CDT pasa su ruta real), pero los `-I` requieren rutas
   fisicas.
4. **IWDG de 4 s (Y5 original) es inviable en este firmware:**
   - `SIGFOX_API_send_frame` bloquea ~7-9 s en RC2 (3 frames de 12 bytes a
     600 bps) → el watchdog cortaria cada transmision.
   - El build define `LPUART` → `LOW_POWER_DISABLE=0` → el MCU duerme en Stop2
     en idle, donde nadie refresca y el IWDG (que corre desde LSI) resetearia
     el equipo ciclicamente. Solucion definitiva para M4: option byte
     `IWDG_STOP = frozen` o refresh al despertar. Por ahora el IWDG queda
     compilado pero deshabilitado por default (`LOCKCONTROL_IWDG_ENABLE=0`,
     timeout 16 s cuando se habilita).

---

## Parche 1 — `Core/Inc/stm32wlxx_hal_conf.h`

Habilitar los modulos HAL que usa el BSP:

```c
/* antes */  /*#define HAL_I2C_MODULE_ENABLED   */
/* ahora */  #define HAL_I2C_MODULE_ENABLED

/* antes */  /*#define HAL_IWDG_MODULE_ENABLED   */
/* ahora */  #define HAL_IWDG_MODULE_ENABLED
```

## Parche 2 — `Core/Inc/utilities_def.h`

Task IDs del sequencer para los eventos de sensores (Y3), dentro del bloque
USER CODE del enum `CFG_SEQ_Task_Id_t`:

```c
  /* USER CODE BEGIN CFG_SEQ_Task_Id_t */
  CFG_SEQ_Task_ACCEL_EVENT,    /* 0G LockControl: evento acelerometro LIS2DW12 (Y3) */
  CFG_SEQ_Task_REED_EVENT,     /* 0G LockControl: evento sensor magnetico (Y3) */
  /* USER CODE END CFG_SEQ_Task_Id_t */
```

## Parche 3 — `Core/Src/main.c`

Tres inserciones (delimitadas con USER CODE):

1. **Includes** (tras `#include "radio_board_if.h"`): `bsp_lockcontrol.h` y
   `app_sensors.h`.
2. **Init del BSP** (tras `SystemClock_Config()`, antes de `SystemApp_Init()`):
   `BSP_LockControl_Init();` — I2C1 + EXTI0/1 + IWDG opcional. Los sensores NO
   se inicializan aqui: `app_sensors_init()` corre en `MX_Sigfox_Init` porque
   necesita el trace UART y el sequencer ya operativos.
3. **`HAL_GPIO_EXTI_Callback`** (bloque `USER CODE BEGIN 4`): despacha PA0 →
   `app_sensors_notify_accel_isr()` y PA1 → `app_sensors_notify_reed_isr()`.

> Nota de diseno: el handoff original ponia `g_accel`/`g_reed` como globales de
> `main.c`. Se encapsularon como estaticos de `app_sensors.c` — main.c queda
> minimo y toda la logica de sensores vive en el repo principal versionado.

## Parche 4 — `Sigfox/App/app_sigfox.c`

1. **Includes** (USER CODE Includes): `bsp_lockcontrol.h`, `app_sensors.h`.
2. **`MX_Sigfox_Init`** (bloque `MX_Sigfox_Init_2`): `app_sensors_init();`
3. **`MX_Sigfox_Process`** (bloque `MX_Sigfox_Process_1`, antes de
   `UTIL_SEQ_Run`):
   ```c
   BSP_LockControl_Watchdog_Refresh();   /* no-op si LOCKCONTROL_IWDG_ENABLE=0 */
   app_sensors_poll();                   /* Y2: red de seguridad de flags */
   ```

## Parche 5 — `STM32CubeIDE/.project`

Nuevos linked resources (mismo patron que los drivers existentes):

| Recurso virtual | Destino fisico |
|---|---|
| `Application/User/app/app_sensors.c` | `PARENT-7-PROJECT_LOC/0G-LockControl-LSM110A/Firmware/app/app_sensors.c` |
| `Application/User/app/app_sensors.h` | `.../Firmware/app/app_sensors.h` |
| `Application/User/app/payload_codec.c` | `.../Firmware/app/payload/payload_codec.c` |
| `Application/User/app/payload_codec.h` | `.../Firmware/app/payload/payload_codec.h` |
| `Drivers/STM32WLxx_HAL_Driver/stm32wlxx_hal_i2c.c` | `PROJECT_ROOT/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_i2c.c` |
| `Drivers/STM32WLxx_HAL_Driver/stm32wlxx_hal_i2c_ex.c` | `.../Src/stm32wlxx_hal_i2c_ex.c` |
| `Drivers/STM32WLxx_HAL_Driver/stm32wlxx_hal_iwdg.c` | `.../Src/stm32wlxx_hal_iwdg.c` |

## Parche 6 — `STM32CubeIDE/.cproject`

Include paths **fisicos** anadidos al GCC (los `../Application/User/...`
virtuales no resuelven — hallazgo 3). Relativos al dir de build (`Debug/`):

```
../../../../../../../../0G-LockControl-LSM110A/Firmware/drivers
../../../../../../../../0G-LockControl-LSM110A/Firmware/app
../../../../../../../../0G-LockControl-LSM110A/Firmware/app/payload
../Application/User/app        (cosmetico, junto al de drivers ya existente)
```

---

## Reproduccion tras re-clone del SDK

1. Aplicar parches 1-4 (código) y 5-6 (proyecto CubeIDE) en ese orden.
2. Verificar con build headless:
   ```bash
   /Applications/STM32CubeIDE.app/Contents/MacOS/STM32CubeIDE \
     --launcher.suppressErrors -nosplash \
     -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
     -data /tmp/ws-limpio \
     -import "$HOME/GitHub/LSM110A/Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE" \
     -build "LSM1x0A_SDK_LoRaWAN_Sigfox/Debug"
   ```
3. Esperado: `0 errors`, 6 warnings pre-existentes del SDK, y objetos en
   `Debug/Application/User/app/` y `Debug/Application/User/drivers/`.
