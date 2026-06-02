# Drivers

Drivers de bajo nivel para los sensores del 0G LockControl LSM110A.

## LIS2DW12 (acelerometro)

Archivo: `lis2dw12.c` / `lis2dw12.h`

Driver minimalista I2C. Pensado para escenario wake-up: el accel duerme
en low-power, y dispara INT1 cuando detecta movimiento sobre el umbral.

### API en 3 llamadas

```c
#include "lis2dw12.h"

extern I2C_HandleTypeDef hi2c1;   // generado por CubeMX

lis2dw12_t accel;
lis2dw12_init_handle(&accel, &hi2c1);

// 1. Verificar comunicacion I2C
uint8_t id;
if (lis2dw12_who_am_i(&accel, &id) != LIS2DW12_OK) {
    // I2C roto o el chip no responde 0x44
    Error_Handler();
}

// 2. Configurar wake-up con threshold 200 mg
if (lis2dw12_config_wakeup(&accel, 200) != LIS2DW12_OK) {
    Error_Handler();
}

// 3. Despues de la EXTI en PA0, leer la fuente:
uint8_t src;
lis2dw12_read_wake_source(&accel, &src);
if (src & LIS2DW12_WU_SRC_WU_IA) {
    // hubo evento de wake-up
    // bits LIS2DW12_WU_SRC_X / _Y / _Z indican el eje dominante
}
```

### Como integrarlo al proyecto CubeIDE del SDK

El SDK SJI vive en `~/GitHub/LSM110A`. El proyecto Eclipse esta en
`Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE/`.

Pasos para sumar este driver al proyecto:

1. En CubeIDE, click derecho sobre el proyecto -> `New` -> `Folder`.
   Nombrelo `drivers` colgando de `Application/User/`.
2. Click derecho sobre la carpeta -> `Import` -> `General` ->
   `File System` -> seleccionar `Firmware/drivers/` del repo principal.
   Marcar `lis2dw12.c` y `lis2dw12.h`. Importante: en `Advanced`,
   marcar **"Create links in workspace"** para que no se copie,
   sino que apunte al archivo del repo principal (asi el SDK queda
   limpio y este driver vive bajo control de version en
   `0G-LockControl-LSM110A`).
3. En `Project -> Properties -> C/C++ General -> Paths and Symbols`,
   agregar la ruta del header a los include paths.
4. En CubeMX (.ioc del proyecto, si esta) habilitar I2C1 con
   PA9=SCL, PA10=SDA, fast mode 400 kHz. Habilitar EXTI0 para PA0
   con trigger rising edge.
5. Build. Si compila, agregar las 3 llamadas a `app_sigfox.c`.

### Notas de diseño

- **Umbral**: configurable en runtime. Spec dice 200 mg default. La
  resolucion en FS=2g es ~31 mg/LSB (6 bits).
- **Consumo**: en low-power mode 1 a 12.5 Hz, el LIS2DW12 consume
  ~1 uA. El MCU queda en Stop2.
- **Anti-rebote**: `WAKE_UP_DUR` esta en 0 (pulso instantaneo). Si en
  campo se ven falsos positivos, subir a 1-2 (cuenta ODR cycles).
