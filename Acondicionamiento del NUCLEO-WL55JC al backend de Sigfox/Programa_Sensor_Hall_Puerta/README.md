# Programa Sensor Hall Puerta (NUCLEO-WL55JC1 → Sigfox)

Monitoreo de **apertura de puerta** con sensor magnético **sin polaridad** (contacto reed de 2 hilos o Hall omnipolar AH1815/SL353/DRV5032-FB/FC). Envía uplink Sigfox al **ABRIR y al CERRAR**, por **interrupción EXTI de ambos flancos** — ya no por sondeo como `Programa_Sensor_Hall_Magnetico`.

Backend Sigfox de **0G IoT Solutions** (previamente WND México) — https://0giotsolutions.com/

---

## ⚠️ Este programa es un proyecto STANDALONE (regla nueva)

A diferencia de los programas anteriores (parches sobre el `Sigfox_AT_Slave` compartido), este es el **primer programa bajo la regla "1 programa = 1 proyecto = 1 `.ioc` = 1 app"**:

- El proyecto CubeIDE completo vive en **`~/Documents/Programa_Sensor_Hall_Puerta/`** (árbol autocontenido: Drivers + Middlewares con libs Sigfox + Utilities + proyecto). GitHub solo documenta el código.
- El pin del sensor se declara en el **`.ioc` propio** y CubeMX genera `gpio.c` / EXTI1 / NVIC — cero hacks manuales de interrupciones.
- Los módulos de otros programas (buttons/distance/hall/flow) **no existen** en este proyecto.
- Esta carpeta contiene el módulo completo (`src/*.c/.h`) y los deltas vs el AT_Slave base (`src/*.patch`) para entender/reproducir el proyecto.

## Contenido de la carpeta

```
Programa_Sensor_Hall_Puerta/
├── README.md
├── backend/
│   └── sigfox_time_server.py    Callback BIDIR de hora (pruebas laptop+túnel)
└── src/
    ├── hall_door_app.c / .h     Módulo de la app (EXTI→debounce→task→TX)   [NUEVO -> Sigfox/App]
    ├── app_features.h           Un solo flag: USE_DOOR_APP                  [NUEVO -> Core/Inc]
    ├── ioc.patch                PA1=HALL_DOOR ambos flancos + NVIC EXTI1    [MOD -> .ioc]
    ├── sgfx_app.c.patch         DoorHall_Init() en Sigfox_Init()            [MOD]
    ├── utilities_def.h.patch    Task CFG_SEQ_Task_DoorTx                    [MOD]
    ├── stm32wlxx_it.c.patch     EXTI1 generado + callback → módulo          [MOD]
    ├── gpio.c.patch             PA1 rising+falling + NVIC (como CubeMX)     [MOD]
    ├── main.h.patch             Defines HALL_DOOR_Pin                       [MOD]
    ├── at_time.patch            Comando AT$TIME=HH:MM:SS (hora del payload) [MOD]
    └── dot_project.patch        Linked resource de hall_door_app.c          [MOD]
```

## Hardware / cableado

Contacto seco de 2 hilos (reed de puerta) — sin polaridad, sin alimentación:

| Sensor | Pin Nucleo | Header |
|--------|-----------|--------|
| Hilo 1 | **PA1**   | Morpho **CN10 pin 36** |
| Hilo 2 | GND       | Morpho **CN10 pin 20** |

- Pull-up **interno** activado desde el `.ioc`. Imán presente (puerta cerrada) = contacto cerrado = PA1 en BAJO.
- **B2 comparte PA1**: sirve de simulador sin cablear nada (presionado = puerta cerrada).
- Hall omnipolar de 3 pines: VDD→3V3, GND→GND, OUT→PA1 (misma lógica activo-bajo).
- Si el reed fuera normalmente-cerrado (lectura invertida): `DOOR_ACTIVE_LOW = 0` en `hall_door_app.h`.
- Instalación real con cable largo: filtro RC en el pin (10 kΩ a 3V3 + 1 nF a GND).

## Cómo funciona

```
EXTI (cualquier flanco en PA1)
  └─► re-arma DebounceTimer (one-shot 200 ms)
        └─► agenda CFG_SEQ_Task_DoorTx en el sequencer
              └─► lee nivel ESTABLE; si cambió vs último TX:
                    ├─ ventana libre → TX Sigfox + arma GapTimer (10 s)
                    └─ en ventana   → queda pendiente; al expirar GapTimer
                                      se re-chequea y se reporta el estado FINAL
```

- Una ráfaga de aperturas/cierres colapsa en **un solo uplink coherente** (el estado final nunca se pierde).
- Tope diario `DOOR_MAX_TX_PER_DAY = 100` (contrato RC2: 140/día) + reset por timer de 24 h.
- La interfaz AT (LPUART1 115200) sigue activa en paralelo.

### Parámetros (`hall_door_app.h`)

| Define | Default | Controla |
|---|---|---|
| `DOOR_DEBOUNCE_MS` | 200 | Estabilidad tras el último flanco |
| `DOOR_TX_MIN_GAP_MS` | 10000 | Ventana mínima entre uplinks |
| `DOOR_MAX_TX_PER_DAY` | 100 | Tope diario (0 = sin tope) |
| `DOOR_TX_STARTUP_STATE` | 0 | 1 = reporta estado inicial al arrancar |
| `DOOR_ACTIVE_LOW` | 1 | Nivel con imán presente |

## Payload Sigfox (6 bytes)

| Byte | Contenido |
|------|-----------|
| 0    | bit1 = estado (**0=ABIERTA / 1=CERRADA**) · bit0 = **hora confiable** (1 = reloj sincronizado por downlink). Valores: `0x00`/`0x01` abierta, `0x02`/`0x03` cerrada |
| 1–3  | Hora del evento **HH MM SS** (hora local, `DOOR_TZ_OFFSET_S` = UTC-6) |
| 4–5  | Contador de eventos confirmados (uint16 BE, con wrap) |

Ej.: `03 10 05 2A 00 07` = CERRADA a las 16:05:42 con hora confiable, 7.º evento. El uplink `F0` es la **petición de hora** (no es evento de puerta). El backend además timestampea cada mensaje del lado servidor. En el producto LSM110A este evento migra al payload de 12 bytes (spec §4.4, byte 5 = estado magnético).

**Hora automática — 3 niveles** (todo en `hall_door_app.c`, flags en el `.h`):

1. **Downlink Sigfox**: al boot (+8 s) manda `F0` con `initiate_downlink_flag`; el backend responde **8 bytes = epoch Unix UTC uint32 BE + 4 reservados** → reloj exacto con fecha, guardado en hora local. Sin respuesta: reintento cada 24 h; sincronizado: re-sync cada `DOOR_TIME_RESYNC_DAYS` (7). Máximo 1 downlink/día de los 4 que permite Sigfox. **Requiere callback BIDIR en el backend** (pendiente de configurar).
2. **Siembra de compilación** (`__TIME__`): automática al arrancar si el reloj viene en cero (best-effort, bit0=0, base epoch-2000).
3. **Manual**: `AT$TIME=HH:MM:SS` / `AT$TIME?` (ver `src/at_time.patch`; best-effort, sin fecha).

El reloj sobrevive resets/re-flasheos (backup domain); un power-cycle regresa a la siembra hasta el siguiente downlink. Umbral de confiabilidad: epoch ≥ 2025-01-01 (`DOOR_CLOCK_REAL_EPOCH_MIN`), que solo el downlink puede producir. **Importante**: el reset NO corrige un desfase de la siembra — el reloj sobrevive con todo y su error (que nace del gap compilar→encender); la hora exacta solo llega por downlink (o `AT$TIME`).

### Configurar el callback BIDIR (hora por downlink) — pruebas con laptop

1. **Servidor** (Terminal 1):
   ```
   python3 backend/sigfox_time_server.py     # escucha en :8000, solo stdlib
   ```
2. **Túnel público** (Terminal 2), cualquiera de los dos:
   ```
   ngrok http 8000                                        # requiere cuenta
   cloudflared tunnel --url http://localhost:8000         # sin cuenta
   ```
   Copiar la URL `https://...` que entregue.
3. **backend.sigfox.com** → Device Type del dispositivo → **Callbacks → New → Custom callback**:

   | Campo | Valor |
   |---|---|
   | Type | **DATA / BIDIR** |
   | Channel | URL |
   | Url pattern | `https://<tu-tunel>/sigfox/time` |
   | HTTP method | POST |
   | Content-Type | `application/json` |
   | Body | `{"device":"{device}","time":{time},"data":"{data}","ack":{ack}}` |

   ⚠️ En la lista de callbacks, marcar el **radiobutton de la columna "Downlink"** en este callback, y en el Device Type verificar **Downlink data = CALLBACK**.
4. Reset a la Nucleo → a los ~8 s pide (`F0`), el servidor loguea la petición y responde el epoch → en la serie: `Reloj sincronizado por downlink: HH:MM:SS (local, UTC-6)`. Desde ahí los eventos salen con bit0=1 y hora exacta.

El servidor responde `{"<device>":{"downlinkData":"<epoch hex 8c>00000000"}}` solo cuando `ack=true` (petición `F0`); a los uplinks normales responde 204 sin downlink. Para producción, mover la misma lógica a una función serverless o UnaConnect (URL permanente).

## Compilación / flasheo

1. Importar `~/Documents/Programa_Sensor_Hall_Puerta/` en CubeIDE (*Import → Existing Projects*, sin copiar).
2. Build → Run (ST-LINK). El linker ya trae el fix **NOLOAD**: las credenciales Sigfox en `0x0803E500` sobreviven al re-flasheo (ver `Persistencia_Credenciales_NOLOAD/`).
3. Serie 115200: banner `DOOR HALL APP LISTA` + eventos `[DOOR] >> TX evento #N: puerta CERRADA/ABIERTA`.

> Nota macOS: si CubeIDE pide "ST-Link Server", instalar el paquete **ST-LINK-SERVER** de st.com; si el GUI no lo encuentra (macOS 15+), lanzar CubeIDE desde Terminal.

---
*Fecha: Julio 2026 · Autor: Yahir Flores · Base: Sigfox_AT_Slave V1.5.0 (NUCLEO-WL55JC1, RC2 México)*
*Empresa: 0G IoT Solutions (previamente WND México) — https://0giotsolutions.com/*
