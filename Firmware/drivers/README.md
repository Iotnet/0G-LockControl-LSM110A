# Drivers

Drivers de bajo nivel para los sensores del 0G LockControl LSM110A.

## Archivos

| Archivo | Sensor | Pin | Funcion |
|---|---|---|---|
| `lis2dw12.c/.h` | Acelerometro LIS2DW12 | PA0 (INT1) + PA9/PA10 (I2C1) | Wake-up por movimiento |
| `reed_switch.c/.h` | Reed switch / DRV5032 | PA1 (EXTI1) | Apertura puerta |

---

## Uso conjunto — patron ISR + main loop

Ambos drivers siguen el mismo patron:
1. Una funcion `*_on_interrupt()` se llama desde el callback EXTI (contexto ISR, sin I/O).
2. Una funcion `*_process_event()` se llama desde el main loop para leer registros / pines y decidir si transmitir.

### 1. Configurar pines en CubeMX (.ioc del proyecto SDK)

| Pin | Modo | Pull | EXTI |
|---|---|---|---|
| PA0 | GPIO_EXTI0 | Pull-down | Rising edge |
| PA1 | GPIO_EXTI1 | Pull-up   | Both edges  |
| PA9 | I2C1_SCL | None | — |
| PA10 | I2C1_SDA | None | — |

Habilitar `EXTI Line0 interrupt` y `EXTI Line1 interrupt` en NVIC con prioridad media (ej. 5).
Habilitar `I2C1` en Fast Mode (400 kHz).

### 2. En `app_sigfox.c` (o `main.c` segun donde manejes la app)

```c
#include "lis2dw12.h"
#include "reed_switch.h"

extern I2C_HandleTypeDef hi2c1;

static lis2dw12_t   g_accel;
static reed_switch_t g_reed;

void app_sensors_init(void)
{
    /* Acelerometro */
    lis2dw12_init_handle(&g_accel, &hi2c1);

    uint8_t id = 0;
    if (lis2dw12_who_am_i(&g_accel, &id) != LIS2DW12_OK) {
        /* I2C roto o chip no responde 0x44 */
        Error_Handler();
    }
    lis2dw12_config_wakeup(&g_accel, 200);   /* 200 mg default */

    /* Reed switch */
    reed_switch_init(&g_reed, GPIOA, GPIO_PIN_1, 50);  /* 50 ms debounce */
}

void app_sensors_poll(void)
{
    /* Eventos del acelerometro */
    if (lis2dw12_has_pending_event(&g_accel)) {
        lis2dw12_event_t evt;
        if (lis2dw12_process_event(&g_accel, &evt) == LIS2DW12_OK
            && evt.detected) {
            /* TODO: armar payload con bit0 = 1 (accel) y enviar Sigfox */
        }
    }

    /* Eventos del reed switch */
    if (reed_switch_has_pending_event(&g_reed)) {
        reed_state_t st;
        reed_switch_process_event(&g_reed, &st);
        /* TODO: armar payload con bit1 = 1 (magnetico) y estado st */
    }
}
```

### 3. En `stm32wlxx_it.c` o donde tengas el callback EXTI

El HAL provee un weak `HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)`. Override en
**un solo lugar** del proyecto. Sugerido: en `app_sigfox.c` o en un nuevo
`event_dispatch.c` colgando de `Application/User/`.

```c
extern lis2dw12_t   g_accel;
extern reed_switch_t g_reed;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        lis2dw12_on_interrupt(&g_accel);
    }
    else if (GPIO_Pin == GPIO_PIN_1) {
        reed_switch_on_interrupt(&g_reed);
    }
}
```

### 4. En el main loop (o stm32_seq task)

Llamar `app_sensors_poll()` periodicamente, o despues de salir de Stop2 por
EXTI. Si usas el scheduler del SDK (`stm32_seq.h`), registra `app_sensors_poll`
como una tarea y dispara `UTIL_SEQ_SetTask` desde el callback EXTI.

---

## Integracion al proyecto CubeIDE del SDK

El SDK SJI vive en `~/GitHub/LSM110A`. El proyecto Eclipse esta en:

```
Projects/NUCLEO-WL55JC/Applications/LoRaWAN_SigFox/LSM1x0A/STM32CubeIDE/
```

Pasos para sumar estos drivers al proyecto:

1. En CubeIDE, click derecho sobre el proyecto -> `New` -> `Folder`.
   Nombrelo `drivers` colgando de `Application/User/`.
2. Click derecho sobre la carpeta -> `Import` -> `General` ->
   `File System` -> seleccionar `Firmware/drivers/` del repo principal.
   Marcar los 4 archivos `.c/.h`. En `Advanced`, marcar **"Create links
   in workspace"** para que no se copien — asi quedan versionados
   en `0G-LockControl-LSM110A`.
3. `Project -> Properties -> C/C++ General -> Paths and Symbols -> Includes`:
   agregar la ruta de los headers.
4. Habilitar I2C1 + EXTI0/1 en el `.ioc` (ver tabla arriba) y
   regenerar el codigo.
5. Build. Debe quedar 0 errores.

---

## Notas de diseño

### LIS2DW12
- Umbral configurable en runtime (default 200 mg, spec sec 4.4).
- En FS=2g, 1 LSB ≈ 31 mg (6 bits).
- En low-power mode 1 a 12.5 Hz consume ~1 uA.
- Anti-rebote: `WAKE_UP_DUR=0` (instantaneo). Subir a 1-2 si hay falsos.

### Reed switch / DRV5032
- Debounce por software con `HAL_GetTick()`. Default 50 ms.
- Misma logica para reed y DRV5032 (ambos open-drain con pull-up).
- Convencion: LOW = iman presente = CLOSED. HIGH = sin iman = OPEN = alarma.
