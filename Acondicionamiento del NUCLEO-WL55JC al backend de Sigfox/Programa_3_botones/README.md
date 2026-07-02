# Programa de los 3 botones — Envío Sigfox por push-button

Documentación técnica del código que añade envíos Sigfox disparados por los botones **B1**, **B2** y **B3** de la placa **NUCLEO-WL55JC**, manteniendo en paralelo la interfaz AT del firmware `Sigfox_AT_Slave` V1.5.0.

- **B1 (PA0)** → payload `"Hola"` = hex `48 6F 6C 61` (4 bytes)
- **B2 (PA1)** → payload `"Mundo"` = hex `4D 75 6E 64 6F` (5 bytes)
- **B3 (PC6)** → payload `"0G"` = hex `30 47` (2 bytes)

Al presionar los 3 botones en secuencia se forma la frase **"Hola Mundo 0G"** — demo end-to-end del backend Sigfox operado por **0G IoT Solutions**.

---

## Contenido

- [Arquitectura general](#arquitectura-general)
- [Mapa de pines](#mapa-de-pines)
- [Flujo de ejecución paso a paso](#flujo-de-ejecución-paso-a-paso)
- [Debounce, rate-limit y feedback visual](#debounce-rate-limit-y-feedback-visual)
- [Archivos nuevos](#archivos-nuevos)
- [Archivos modificados y por qué](#archivos-modificados-y-por-qué)
- [Cómo compilar](#cómo-compilar)
- [Cómo probar](#cómo-probar)
- [Interacción con la interfaz AT](#interacción-con-la-interfaz-at)
- [Notas de bajo consumo](#notas-de-bajo-consumo)

---

## Arquitectura general

El diseño sigue el patrón oficial del SDK **STM32CubeWL** con **stm32_seq** (sequencer cooperativo de bajo consumo). El flujo de un click es:

```
┌────────────┐  presión física
│   BOTÓN    │──────────────┐
│  (Bx)      │              │
└────────────┘              ▼
                   ┌──────────────────────┐
                   │  EXTI0/EXTI1/EXTI9_5 │ ISR corta
                   │  IRQ Handler         │ (~microsegundos)
                   └──────────┬───────────┘
                              │
                              ▼
                   ┌──────────────────────┐
                   │ HAL_GPIO_EXTI_       │ limpia pending bit
                   │ Callback()           │ delega en app
                   └──────────┬───────────┘
                              │
                              ▼
                   ┌──────────────────────┐
                   │ Buttons_HandleEXTI() │ debounce SW
                   │ [contexto ISR]       │ setea pendingButton
                   └──────────┬───────────┘
                              │
                              ▼
                   ┌──────────────────────┐
                   │ UTIL_SEQ_SetTask()   │ marca tarea pendiente
                   │ (retorna a main loop)│
                   └──────────┬───────────┘
                              │
              ═══════════════ ISR termina ═══════════════
                              │
                              ▼
                   ┌──────────────────────┐
                   │ Buttons_Process()    │ CONTEXTO MAIN LOOP
                   │ (sequencer task)     │ Verifica rate-limit
                   └──────────┬───────────┘
                              │
                              ▼
                   ┌──────────────────────┐
                   │ SIGFOX_API_send_     │ Bloquea ~6 s
                   │ frame()              │ TX1/TX2/TX3 en 902 MHz
                   └──────────┬───────────┘
                              │
                              ▼
                   ┌──────────────────────┐
                   │ LEDs + log en VCP    │ Feedback visual
                   └──────────────────────┘
```

**Principio clave:** la ISR **NO** llama directamente a `SIGFOX_API_send_frame()`. Esa función bloquea ~6 segundos mientras la radio transmite las 3 réplicas — hacerlo en contexto ISR bloquearía el resto del sistema. En su lugar, la ISR sólo _marca_ que hay una TX pendiente y retorna. El main loop, vía el sequencer `stm32_seq`, ejecuta la TX en contexto de usuario.

---

## Mapa de pines

Verificado desde `Core/Inc/main.h` del proyecto:

| Función | Pin MCU | EXTI line | ISR handler |
|---|---|---|---|
| Botón B1 | PA0 | EXTI0 | `EXTI0_IRQHandler` |
| Botón B2 | PA1 | EXTI1 | `EXTI1_IRQHandler` |
| Botón B3 | PC6 | EXTI6 (grupo [5:9]) | `EXTI9_5_IRQHandler` |
| LED azul LD1 | PB15 | — | (output) |
| LED verde LD2 | PB9 | — | (output) |
| LED rojo LD3 | PB11 | — | (output) |

Los 3 botones ya venían **configurados** desde el `gpio.c` del proyecto original (`GPIO_MODE_IT_FALLING` + `GPIO_PULLUP`). Lo que faltaba —y que el código nuevo agrega— es habilitar las NVIC interrupts, escribir los handlers, y la lógica de aplicación.

---

## Flujo de ejecución paso a paso

### 1. Inicialización (una sola vez, al boot)

En `Sigfox_Init()` de `sgfx_app.c` se agrega la llamada a `Buttons_Init()`. Dentro se ejecuta:

```c
void Buttons_Init(void)
{
  /* Apaga los 3 LEDs */
  LED_BUSY_OFF(); LED_OK_OFF(); LED_ERR_OFF();

  /* Habilita NVIC para las 3 lineas EXTI */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);   HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);   HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0); HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* Registra la tarea del sequencer */
  UTIL_SEQ_RegTask((1U << CFG_SEQ_Task_ButtonTx), UTIL_SEQ_RFU, Buttons_Process);

  APP_PPRINTF("BOTONES LISTOS: B1=Hola  B2=Mundo  B3=0G\r\n");
}
```

**Prioridad NVIC 5:** intermedia. No pisa al radio Sigfox (que corre en prioridades más altas, típicamente 0-3) y garantiza que la ISR se ejecute rápido después de una presión.

### 2. Presión del botón (evento asíncrono)

Cuando el usuario presiona físicamente un botón:

1. La línea EXTI genera IRQ.
2. El **NVIC** llama al handler correspondiente en `stm32wlxx_it.c`:

   ```c
   void EXTI0_IRQHandler(void) {  HAL_GPIO_EXTI_IRQHandler(BUT1_Pin);  }
   ```

3. `HAL_GPIO_EXTI_IRQHandler()` (de HAL, no lo escribimos nosotros):
   - Limpia el pending bit de EXTI.
   - Llama a la función `__weak` `HAL_GPIO_EXTI_Callback(GPIO_Pin)`.

4. Nuestro override de `HAL_GPIO_EXTI_Callback()` (también en `stm32wlxx_it.c`):

   ```c
   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
     Buttons_HandleEXTI(GPIO_Pin);
   }
   ```

5. `Buttons_HandleEXTI()` (en `buttons_app.c`):

   ```c
   void Buttons_HandleEXTI(uint16_t GPIO_Pin)
   {
     uint32_t now = HAL_GetTick();

     /* --- Debounce software --- */
     if ((now - lastIsrTick) < BTN_DEBOUNCE_MS) return;
     lastIsrTick = now;

     /* --- Mapea pin a ID --- */
     ButtonId_t btn = BTN_NONE;
     if (GPIO_Pin == BUT1_Pin)      btn = BTN_B1;
     else if (GPIO_Pin == BUT2_Pin) btn = BTN_B2;
     else if (GPIO_Pin == BUT3_Pin) btn = BTN_B3;
     if (btn == BTN_NONE) return;

     /* --- Set flag + agenda tarea --- */
     if (pendingButton == BTN_NONE) {
       pendingButton = btn;
       UTIL_SEQ_SetTask((1U << CFG_SEQ_Task_ButtonTx), CFG_SEQ_Prio_0);
     }
   }
   ```

   Puntos clave:

   - **Debounce por software:** si dos ISR llegan en < 200 ms, se descarta la segunda. Los pulsadores mecánicos rebotan varios milisegundos al cierre — sin debounce, un solo click podría ser leído como 2-3 pulsaciones y disparar múltiples uplinks (**cada uno consume 1 token del contrato Sigfox**).
   - **Descartar dobles:** si `pendingButton` ya tiene un ID, no se sobreescribe. La primera pulsación gana.
   - **No hay `printf` ni operaciones largas** en la ISR — solo comparaciones y una asignación atómica.

6. `UTIL_SEQ_SetTask()` marca el bit correspondiente al `CFG_SEQ_Task_ButtonTx` en un bitmask global del sequencer. La ISR termina inmediatamente y el MCU vuelve al main loop.

### 3. Ejecución de la tarea (contexto main loop)

En el ciclo `while(1)` de `main.c` se llama continuamente a `MX_Sigfox_Process()`, que a su vez llama a `UTIL_SEQ_Run(UTIL_SEQ_DEFAULT)`. Éste chequea el bitmask, ve que `CFG_SEQ_Task_ButtonTx` está pendiente y ejecuta `Buttons_Process()`:

```c
static void Buttons_Process(void)
{
  ButtonId_t btn = pendingButton;
  pendingButton = BTN_NONE;                 /* Listo para el siguiente click */
  if (btn == BTN_NONE) return;

  /* --- Rate limit --- */
  uint32_t now = HAL_GetTick();
  if (lastTxTick != 0U && (now - lastTxTick) < BTN_RATE_LIMIT_MS) {
    APP_PPRINTF("[BTN] Rate limit activo. Faltan %u ms\r\n",
                (unsigned)(BTN_RATE_LIMIT_MS - (now - lastTxTick)));
    Buttons_BlinkBlue(6);                   /* Feedback: 6 parpadeos rapidos */
    return;
  }

  Buttons_SendPayload(btn);
}
```

**Rate limit:** rechaza el envío si han pasado menos de **30 segundos** desde el último uplink exitoso. Diseño intencional: cada uplink consume un token del contrato Sigfox `UnaTag_test` (recordar el budget limitado del device de laboratorio). El rate limit previene el gasto masivo si alguien presiona en ráfaga.

### 4. Envío efectivo

`Buttons_SendPayload()` es donde se llama al stack Sigfox:

```c
static void Buttons_SendPayload(ButtonId_t button)
{
  const uint8_t *payload; uint8_t length; const char *label;

  switch (button) {
    case BTN_B1: payload = PAYLOAD_B1; length = 4; label = "B1 = Hola";  break;
    case BTN_B2: payload = PAYLOAD_B2; length = 5; label = "B2 = Mundo"; break;
    case BTN_B3: payload = PAYLOAD_B3; length = 2; label = "B3 = 0G";    break;
    default: return;
  }

  APP_PPRINTF("[BTN] >> TX %s (%u bytes)\r\n", label, length);
  LED_OK_OFF(); LED_ERR_OFF();
  LED_BUSY_ON();                                     /* LED rojo durante TX */

  uint8_t dl_msg[8] = {0};
  sfx_error_t err = SIGFOX_API_send_frame(
      (sfx_u8*)payload,
      (sfx_u8)length,
      dl_msg,
      (sfx_u8)BTN_TX_REPLICAS,                       /* 1 replica */
      SFX_FALSE                                      /* Sin downlink request */
  );

  LED_BUSY_OFF();

  if (err == SFX_ERR_NONE) {
    LED_OK_ON();                                     /* LED verde */
    lastTxTick = HAL_GetTick();                      /* Arranca rate-limit */
    APP_PPRINTF("[BTN] << TX OK\r\n");
    HAL_Delay(BTN_LED_OK_HOLD_MS);
    LED_OK_OFF();
  } else {
    LED_ERR_ON();                                    /* LED azul */
    APP_PPRINTF("[BTN] << TX ERROR 0x%04X\r\n", err);
    HAL_Delay(BTN_LED_ERR_HOLD_MS);
    LED_ERR_OFF();
  }
}
```

Payloads como constantes en `.rodata` (no ocupan RAM):

```c
static const uint8_t PAYLOAD_B1[] = { 0x48, 0x6F, 0x6C, 0x61 };          /* "Hola" */
static const uint8_t PAYLOAD_B2[] = { 0x4D, 0x75, 0x6E, 0x64, 0x6F };    /* "Mundo" */
static const uint8_t PAYLOAD_B3[] = { 0x30, 0x47 };                       /* "0G" */
```

---

## Debounce, rate-limit y feedback visual

| Parámetro | Valor default | Símbolo (buttons_app.h) | Rationale |
|---|---|---|---|
| Debounce ISR | 200 ms | `BTN_DEBOUNCE_MS` | Suprime rebotes mecánicos y triple-click accidental. |
| Rate limit entre TX | 30 s | `BTN_RATE_LIMIT_MS` | Presupuesto de tokens Sigfox (contrato `UnaTag_test` limitado). |
| Réplicas TX | 1 | `BTN_TX_REPLICAS` | Uplinks de bajo costo. Aumentar a 3 para link más robusto (consume 1 token igual, pero más energía). |
| LED verde tras TX OK | 3 s | `BTN_LED_OK_HOLD_MS` | Confirmación visual, evita duda del operador. |
| LED azul tras TX ERR | 3 s | `BTN_LED_ERR_HOLD_MS` | Idem para error. |

**Codificación de LEDs:**

| LED | Pin | Color | Significa |
|---|---|---|---|
| LD3 | PB11 | Rojo | TX en curso (radio activo) |
| LD2 | PB9 | Verde | TX OK — mensaje aceptado por el stack |
| LD1 | PB15 | Azul | TX error o rate-limit rechazado (6 parpadeos rápidos si es rate-limit) |

---

## Archivos nuevos

### `Sigfox/App/buttons_app.h`

API pública del módulo. Solo declara `Buttons_Init()` y `Buttons_HandleEXTI()`. Ver [`src/buttons_app.h`](src/buttons_app.h) para el contenido completo.

### `Sigfox/App/buttons_app.c`

Implementación completa: init, ISR handler, tarea del sequencer, send payload, blink azul. **Único archivo con lógica de aplicación nueva.** Ver [`src/buttons_app.c`](src/buttons_app.c).

---

## Archivos modificados y por qué

Se hicieron 4 modificaciones puntuales en archivos existentes del SDK. Cada una está documentada con su diff conceptual en la carpeta `src/`:

| Archivo | Cambio | Motivo | Diff |
|---|---|---|---|
| `Core/Inc/utilities_def.h` | +1 línea en enum de tareas del sequencer | Registrar `CFG_SEQ_Task_ButtonTx` para poder invocar `UTIL_SEQ_SetTask()` desde la ISR | [`utilities_def.h.patch`](src/utilities_def.h.patch) |
| `Core/Src/stm32wlxx_it.c` | +37 líneas: 3 EXTI handlers + `HAL_GPIO_EXTI_Callback` | El proyecto original NO tenía handlers para EXTI0/1/9_5. Sin ellos, la IRQ dispararía el `Default_Handler` (loop infinito) | [`stm32wlxx_it.c.patch`](src/stm32wlxx_it.c.patch) |
| `Sigfox/App/sgfx_app.c` | 2 líneas: `#include "buttons_app.h"` + llamada a `Buttons_Init()` | Enganchar el módulo al ciclo de inicialización oficial del stack Sigfox | [`sgfx_app.c.patch`](src/sgfx_app.c.patch) |
| `STM32CubeIDE/.project` | +5 líneas: `<link>` a `buttons_app.c` | STM32CubeIDE usa **linked resources**; sin este entry, el `.c` no entra al build (linker tira "undefined reference to Buttons_Init") | [`dot_project.patch`](src/dot_project.patch) |

> **Nota importante:** todos los cambios se realizaron dentro de bloques `USER CODE BEGIN` / `USER CODE END` de CubeMX/CubeIDE. Esto significa que **si en el futuro se regenera el proyecto desde el `.ioc` de CubeMX, las modificaciones se preservan**. Es una práctica obligatoria para no perder trabajo custom al re-generar código.

---

## Cómo compilar

Con el proyecto abierto en STM32CubeIDE:

1. Confirmar que en Project Explorer aparece `buttons_app.c` bajo:
   `Sigfox_AT_Slave/Application/User/Sigfox/App/`
   Si no aparece → click derecho en el proyecto → **Refresh** (F5).
2. Click derecho sobre `Sigfox_AT_Slave` → **Clean Project**.
3. Click derecho → **Build Project** (o `Cmd+B`).

**Salida esperada:**

```
arm-none-eabi-size Sigfox_AT_Slave.elf
   text    data     bss     dec     hex   filename
  ~77500    312    ~10550  ~88K   16XXX   Sigfox_AT_Slave.elf

Build Finished. 0 errors, X warnings.
```

Los `X warnings` son cosméticos del SDK (encoding, unused variable en `lora_at.c` de otra región). Ignorables.

**El firmware compilado queda en:**

```
STM32CubeIDE/Debug/Sigfox_AT_Slave.elf
```

---

## Cómo probar

### 1. Flashear el firmware nuevo

Con STM32CubeProgrammer (o `STM32_Programmer_CLI`):

- Mode: `Under reset` · Reset mode: `Hardware reset` · Connect.
- Erasing & Programming → Browse → `Sigfox_AT_Slave.elf`.
- ✓ Verify programming · ✓ Run after programming · ☐ Skip flash erase.
- Start Programming.

> **⚠️ Credenciales Sigfox:** el fix del linker documentado en la carpeta `Persistencia_Credenciales_NOLOAD/` garantiza que este flasheo **no borre** las credenciales `sigfox_data_<ID>.bin` en `0x0803E500`. Si es la primera vez que flasheas este firmware sobre un chip que aún tenía credenciales TEST, tienes que hacer un `Write data` con el `.bin` real del portal sfxp primero. Ver README de esa carpeta.

### 2. Abrir Serial Monitor

Configuración: **9600 baud · Carriage return · sin flow control**.

**Boot banner esperado:**

```
APPLICATION_VERSION: V1.5.0
MW_SIGFOX_VERSION:   V1.8.0
MW_RADIO_VERSION:    V1.4.0
ATtention command interface

SIGFOX APPLICATION READY

BOTONES LISTOS:
  B1 (PA0) --> Hola  (48 6F 6C 61)
  B2 (PA1) --> Mundo (4D 75 6E 64 6F)
  B3 (PC6) --> 0G    (30 47)
  Debounce: 200 ms  |  Rate limit: 30 s
```

Si aparece **`BOTONES LISTOS`**, el módulo se compiló y se ejecutó correctamente.

### 3. Probar cada botón

Con el device previamente registrado en `backend.sigfox.com`:

1. Presionar **B1** → esperar ~6 s.
2. En el Serial Monitor debe aparecer:
   ```
   [BTN] >> TX B1 = Hola (4 bytes)
   [BTN] << TX OK
   ```
3. En backend Sigfox → **Messages**: aparece nueva fila con payload `486f6c61` en 2-30 s.
4. Esperar **30 segundos** (rate limit).
5. Presionar **B2** → verificar `4d756e646f`.
6. Esperar 30 s. Presionar **B3** → verificar `3047`.

Los 3 mensajes juntos forman la frase **"Hola Mundo 0G"** en el backend.

### 4. Comportamientos secundarios a probar

- **Doble-click rápido en el mismo botón (< 200 ms):** solo debe llegar 1 mensaje al backend (debounce funcionando).
- **Segundo click antes de 30 s desde el primero:** LED azul parpadea 6 veces, Serial imprime `[BTN] Rate limit activo. Faltan XXXX ms`, NO se envía.
- **Comandos AT sin presionar botón:** siguen funcionando en paralelo (`AT`, `AT$ID`, `AT$SF=...`), ver siguiente sección.

---

## Interacción con la interfaz AT

**El módulo de botones NO reemplaza la interfaz AT.** Ambos coexisten:

- La ISR de UART sigue registrada y escuchando comandos.
- La tarea `CFG_SEQ_Task_Vcom` sigue registrada y procesando comandos AT en el main loop.
- La nueva tarea `CFG_SEQ_Task_ButtonTx` se ejecuta en paralelo (no simultáneamente — el sequencer es cooperativo, pero ambas tareas alternan cuando corresponde).

**Casos típicos:**

- Enviar `AT$SF=48656C6C6F` por Serial → envía "Hello" (5 bytes).
- Presionar B1 → envía "Hola" (4 bytes).
- Los dos canales comparten el mismo device ID, KEY, región RC2, y el mismo budget de tokens del contrato Sigfox.

**No debes:** llamar `SIGFOX_API_send_frame()` desde AT mientras un envío por botón está en curso. El stack Sigfox es single-threaded en cuanto al radio. En la práctica, dado que ambos canales atraviesan `SIGFOX_API_send_frame()` en la misma capa de aplicación y el radio se turna via sequencer, no hay condición de carrera si el usuario espera el `OK` de una operación antes de disparar la siguiente.

---

## Notas de bajo consumo

El proyecto original `Sigfox_AT_Slave` usa **stm32_lpm** (Low Power Manager) para poner al MCU en modo **STOP** entre eventos. Esto es lo que causa el mensaje `Warning: Connection to device is lost` en CubeProgrammer post-flasheo — el MCU se durmió y las patas SWD se apagaron.

**Los botones despiertan al MCU correctamente** porque:

1. Las líneas EXTI0/EXTI1/EXTI9_5 son **wakeup-capable** en STM32WL: cualquier IRQ EXTI configurada saca al chip de STOP y ejecuta la ISR normalmente.
2. En particular, **PA0 corresponde a `WAKEUP1`**, uno de los pines más eficientes para wakeup en STM32WL.
3. Después de ejecutar la tarea (`Buttons_Process()` + `SIGFOX_API_send_frame`), el sequencer devuelve control al `MX_Sigfox_Process()` que vuelve a permitir modo bajo consumo hasta el próximo evento.

**Consecuencia:** el firmware es apto para operación con batería. En idle, consumo típico STM32WL55JC en STOP2 con RTC = ~2-5 µA (según datasheet). Durante un TX Sigfox: pico de ~90 mA por ~1.5 s totales (3 réplicas de 500 ms cada una), lo cual promedia bien para presiones ocasionales.

---

## Referencias

- Guía completa (PDF, 29 páginas): [`../Guia_completa.pdf`](../Guia_completa.pdf)
- Fix del linker que protege credenciales al re-flashear: [`../Persistencia_Credenciales_NOLOAD/`](../Persistencia_Credenciales_NOLOAD/)
- ST **AN5480 Rev 8** — How to build a Sigfox application with STM32CubeWL
- ST **UM2609** — STM32CubeIDE user guide (Modificar linker script)
- ST **UM2592** — NUCLEO-WL55JC (MB1389) hardware manual

---

**Autor:** Yahir Flores — `yflores@iotnet.mx`
**Empresa:** 0G IoT Solutions (previamente WND México) — [0giotsolutions.com](https://0giotsolutions.com/)
**Versión:** 1.0 · Julio 2026
