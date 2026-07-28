# GATE 2 — Troubleshooting: "el LED parpadea 6 veces y no transmite"

> Hallazgo de bancada, julio 2026. Bloqueaba la Parte A/B del
> [issue #6](https://github.com/jdiaznxt/0G-LockControl-LSM110A/issues/6).
> Setup: NUCLEO-WL55JC + `Programa_3_botones`, alimentada desde KORAD KA3005D
> con Rs serie + cap 470 µF, **USB desconectado**.

## 1. Síntoma

Al alimentar la placa desde la fuente externa (sin USB), al presionar B1:

- el **LED azul (LD1, PB15) parpadea 6 veces rápido**,
- **no** sale uplink al backend Sigfox,
- después la placa parece "muerta": las siguientes pulsaciones no hacen nada,
- la alimentación es correcta (VDD en reposo ~3.0 V, la fuente no entra en CC).

## 2. Diagnóstico: no es un problema de alimentación

**Los 6 parpadeos son un código de estado del firmware, no un fallo eléctrico.**
Es la única secuencia de 6 parpadeos que existe en todo el repo:

```c
/* buttons_app.c v1.0 — Buttons_Process() */
if (lastTxTick != 0U && (now - lastTxTick) < BTN_RATE_LIMIT_MS) {
    Buttons_BlinkBlue(6);   /* 6 parpadeos rapidos = uplink rechazado */
    return;                 /* <-- se va sin transmitir */
}
```

Significa **rate limit activo**: el firmware rechaza el uplink porque cree que
han pasado menos de 30 s desde el último TX exitoso. Por eso "no manda el
mensaje ni hace nada": el `return` ocurre antes de tocar el radio.

### Por qué el rate limit nunca vencía

`now` y `lastTxTick` venían de `HAL_GetTick()`, que cuenta sobre **SysTick**, y
**SysTick se detiene en Stop2**. Este build duerme en Stop2 en idle
(`LOW_POWER_DISABLE=0`, ver `docs/sdk-patches.md` hallazgo 4), así que entre
eventos el contador de HAL se congela: **30 s de reloj de pared pueden ser
menos de 1 s de ticks**. La ventana de 30 s no vencía nunca en tiempo real.

El mismo congelamiento afectaba al debounce (`lastIsrTick`, misma base de
tiempo), y eso explica la parte de "ni hace nada": con el tick detenido, cada
pulsación posterior caía dentro de los 200 ms de anti-rebote y se descartaba
→ **el botón quedaba muerto**.

### Por qué sólo se ve sin USB

Con el USB conectado la placa está prácticamente siempre despierta (tráfico
del VCP/AT, prints, debugger) → SysTick corre → el rate limit se comporta como
dice el README y el Serial Monitor además imprime `Rate limit activo. Faltan
XXXX ms`. Al quitar el USB —justo la condición que exige el GATE 2— la placa
por fin duerme de verdad, el tick se congela y el bug aparece. Sin serial,
además, el único aviso es el parpadeo.

> El equipo ya había documentado esta trampa en el programa del VL53L0X:
> *"el timer corre sobre RTC y por tanto sigue avanzando en STOP2 (a diferencia
> de HAL_GetTick)"* — `Programa_Sensor_Distancia_VL53L0X/src/distance_app.h`.

## 3. Desbloqueo inmediato (sin reflashear)

Un reset reinicializa `lastTxTick` a 0, y con `lastTxTick == 0` el rate limit
no aplica → la siguiente pulsación **sí** transmite:

1. Pulsar el botón negro **RESET (B4)** de la Nucleo.
2. Pulsar **B1** → transmite y se captura el dip.
3. Para cada corrida de la matriz: **RESET → B1**. Una medición por reset.

Sirve para levantar los datos de hoy. La corrección de firmware es el §4.

## 4. Corrección de firmware (v1.1, ya en el repo)

`Programa_3_botones/src/buttons_app.{c,h}`: el debounce y el rate limit pasan
de `HAL_GetTick()` a **timers `UTIL_TIMER` (base RTC)**, que sí siguen contando
en Stop2 — el mismo patrón `DebounceTimer`/`GapTimer` que ya usa
`hall_door_app.c`. Se eliminó todo uso de `HAL_GetTick()` del módulo.

Además se agregó `BTN_GATE2_MODE` en `buttons_app.h`:

```c
#define BTN_GATE2_MODE   1    /* 0 = demo normal, 1 = medicion GATE 2 */
```

Con `1`: rate limit deshabilitado y holds de LED en 0, para que cada pulsación
transmita y el riel vuelva a reposo en cuanto termina el TX (los 3 s de LED
encendido de la v1.0 roban ~2-3 mA que retrasan la recarga del cap de 470 µF
entre disparos). **Ojo: en este modo cada pulsación consume un token Sigfox.**

## 5. Otros dos ajustes al procedimiento (afectan la validez del dato)

### 5.1 Réplicas TX: usar 3, no 1

`BTN_TX_REPLICAS` está en **1**, pero el producto real transmite **3 frames**
(`SIGFOX_API_send_frame` bloquea 7-9 s en RC2, ver `docs/sdk-patches.md`).
Medir con 1 réplica **subestima** el peor caso: con 3 frames el cap arranca el
3.º ya parcialmente descargado, y ése es el `Vmin` que decide el gate.

```c
#define BTN_TX_REPLICAS   3U
```

Las 3 réplicas son **1 solo mensaje** para el contrato Sigfox — no cuesta
tokens extra, sólo energía. Con réplicas = 1 se ve **un** dip corto; con 3 se
ven **tres** dips en la ventana, y el dato del gate es el del peor.

### 5.2 Límite de corriente: 0.5 A, no 200 mA

El procedimiento de bancada pide poner el límite CC en ~200 mA. Contradice la
guía de este repo (`setup-fuente-variable.md` §4), que pide **0.3–0.5 A**, y
200 mA es demasiado apretado:

- inrush al conectar: `V/Rs = 3.0 V / 22 Ω ≈ 136 mA` sólo para cargar el cap,
- pico de TX documentado en la placa: **~90 mA** (~123 mA si se prueba +22 dBm),
- suma de ambos si se dispara antes de que el cap termine de cargar.

Si la fuente entra en **CC**, su respuesta transitoria distorsiona la forma del
dip (mide la fuente, no la pila) y puede provocar brown-out del MCU. **Quien
limita la corriente es Rs, no la fuente.** El límite CC es sólo red de
seguridad contra un cableado equivocado → dejarlo en 0.5 A.

## 6. Nota: con 470 µF, "quitar la alimentación" no siempre resetea

El tiempo que el riel tarda en caer bajo el BOR (~1.7 V) al cortar la fuente es
`t ≈ C · ΔV / I_reposo`:

| Consumo en reposo | t hasta BOR (470 µF, 3.0 → 1.7 V) |
|---|---|
| Nucleo, ST-LINK sin alimentar (~1 mA) | ~0.6 s |
| LSM110A en Stop2 (~3 µA) | **~200 s** |

En la Nucleo el power-cycle sí resetea. En la **PCB final** el cap mantiene al
módulo vivo minutos: apagar y volver a encender no garantiza un arranque en
frío, y el estado en RAM sobrevive. Para forzar reset de verdad: **NRST**, o
cortocircuitar VDD-GND con la salida apagada. Vale la pena tenerlo presente en
el bring-up del §6 de `docs/sistema-minimo-prototipo-v0.md`.

## 7. Pendiente relacionado (NO corregido aquí)

El **mismo bug de base de tiempo** está en el firmware de producto, y ahí es
más grave:

| Archivo | Línea | Qué usa `HAL_GetTick()` | Efecto en Stop2 |
|---|---|---|---|
| `Firmware/app/app_sensors.c` | ~250 | **cooldown de TX de 60 s** (decisión del spec) | El cooldown no vence → **sólo la primera alarma del arranque se transmite** |
| `Firmware/drivers/reed_switch.c` | ~38 | debounce del reed | El debounce no vence → eventos de puerta descartados |

Es un cambio en la ruta de producto (M4, low power), con decisión de diseño
propia: migrar a `UTIL_TIMER`/`SysTimeGet()` o refrescar el tick al despertar.
No se toca en este parche para no mezclarlo con el desbloqueo del GATE 2 —
**conviene abrirle issue propio antes de cerrar M4**.

---
*Referencias: `setup-fuente-variable.md` (§4 Rs y límite CC), `docs/sdk-patches.md`
(hallazgo 4, `LOW_POWER_DISABLE=0`), `Programa_Sensor_Distancia_VL53L0X/src/distance_app.h`
(RTC vs HAL_GetTick), `Programa_Sensor_Hall_Puerta/src/hall_door_app.c` (patrón
`DebounceTimer`/`GapTimer`), ST UM2592 (NUCLEO-WL55JC, RESET B4 y jumper JP2/IDD).*
