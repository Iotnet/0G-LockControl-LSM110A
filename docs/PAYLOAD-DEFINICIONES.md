# Definiciones del Payload — 0G LockControl (LSM110A)

Documento de referencia que consolida **todas las definiciones del payload
Sigfox** del proyecto (WP-01 "Codec del payload"). Recopila el contrato de bytes,
las constantes, la estructura en memoria, la API de codec (C), el parser (Python)
y los vectores de prueba validados.

> **Fuentes en el repo** (esto es un resumen; el código es la verdad):
> - `Firmware/app/payload/payload_codec.h` — declaraciones + contrato
> - `Firmware/app/payload/payload_codec.c` — encode/decode, C puro sin HAL
> - `App/Backend/payload_parser.py` — parser Python (`parse_payload` / `encode_payload`)
> - `Tests/payload/test_vectors.md` — 5 vectores con tablas y hex
> - `Tests/payload/test_payload_host.c` — test host en C
> - `Firmware/app/payload/VALIDACION.md` — checklist de validación
> - `docs/diseno-logico/` — **diseño lógico del firmware (FSM + clasificación)**,
>   que propone extender este contrato → ver [§9](#9-extensión-propuesta-v2--evento-y-tiempo-abierto)

> ### Dos versiones del contrato en este documento
>
> | | Estado | Dónde |
> |---|---|---|
> | **v1 — vigente** | Implementado y validado 5/5 en código | §1 a §8 |
> | **v2 — propuesta** | Sólo diseño, **no implementado** | [§9](#9-extensión-propuesta-v2--evento-y-tiempo-abierto) y [§10](#10-pendientes-para-implementar-la-v2) |
>
> Lo que hoy corre en `payload_codec.{h,c}` y `payload_parser.py` es la **v1**.
> La v2 no cambia la longitud (siguen 12 bytes): usa los dos bytes reservados y
> refina la semántica de dos campos.

---

## 1. Contrato del payload (v1 — vigente)

- **Longitud fija:** 12 bytes (`PAYLOAD_LEN = 12`).
- **Endianness:** los campos `uint16` se serializan en **big-endian** (MSB primero).
- **Transporte:** Sigfox (payload de usuario de 12 bytes).

| Byte(s) | Campo | Tipo | Definición |
|---------|-------|------|------------|
| **0** | `tipo` | `uint8` | `0x01` alarma · `0x02` heartbeat |
| **1** | `fuente` | `uint8` (bitfield) | bit0 = acelerómetro (`0x01`) · bit1 = magnético (`0x02`); combinables con OR |
| **2–3** | `magnitud_mg` | `uint16` **BE** | Magnitud de aceleración en mg |
| **4** | `eje` | `uint8` | `0` ninguno · `1` X · `2` Y · `3` Z |
| **5** | `magnetico` | `uint8` | `0` cerrado · `1` abierto |
| **6** | `bateria_pct` | `uint8` | Nivel de batería 0..100 |
| **7** | `temp` | `uint8` | `temp_c + 40` (offset para soportar negativos) |
| **8–9** | `conteo` | `uint16` **BE** | Contador de eventos |
| **10–11** | `reservado` | — | Siempre `0x0000` (se ignora al decodificar) |

**Nota sobre temperatura:** el byte 7 lleva el offset `+40`; la temperatura real
`temp_c` puede ser negativa. Con `int8_t` y offset 40 el rango representable es
**−40 .. +87 °C**.

**Bytes 10–11:** hoy reservados a `0x0000`. La v2 los ocupa con `evento` y
`t_abierto_s` — ver [§9](#9-extensión-propuesta-v2--evento-y-tiempo-abierto).

---

## 2. Constantes definidas (`payload_codec.h`)

```c
#define PAYLOAD_LEN              12

/* Byte 0: tipo de mensaje */
#define PAYLOAD_TIPO_ALARMA      0x01
#define PAYLOAD_TIPO_HEARTBEAT   0x02

/* Byte 1: fuente (bitfield, OR-able) */
#define PAYLOAD_FUENTE_NINGUNA     0x00
#define PAYLOAD_FUENTE_ACCEL       0x01   /* bit0 */
#define PAYLOAD_FUENTE_MAGNETICO   0x02   /* bit1 */

/* Byte 4: eje que disparó (acelerómetro) */
#define PAYLOAD_EJE_NINGUNO        0x00
#define PAYLOAD_EJE_X              0x01
#define PAYLOAD_EJE_Y              0x02
#define PAYLOAD_EJE_Z              0x03

/* Byte 5: estado del sensor magnético */
#define PAYLOAD_MAG_CERRADO        0x00
#define PAYLOAD_MAG_ABIERTO        0x01

/* Byte 7: offset de temperatura (temp_c + offset) */
#define PAYLOAD_TEMP_OFFSET        40

/* Códigos de retorno */
#define PAYLOAD_OK                 0
#define PAYLOAD_ERR_NULL          -1   /* puntero nulo en parámetros      */
#define PAYLOAD_ERR_TIPO          -2   /* tipo desconocido al decodificar */
```

---

## 3. Estructura en memoria (`payload_t`)

Representación con los campos **ya interpretados** (sin offset, valores reales):

```c
typedef struct {
    uint8_t  tipo;            /* PAYLOAD_TIPO_*                             */
    uint8_t  fuente;          /* OR de PAYLOAD_FUENTE_*                     */
    uint16_t magnitud_mg;     /* magnitud de aceleración en mg             */
    uint8_t  eje;             /* PAYLOAD_EJE_*                             */
    uint8_t  magnetico;       /* PAYLOAD_MAG_*                             */
    uint8_t  bateria_pct;     /* nivel de batería 0..100                   */
    int8_t   temp_c;          /* temperatura real en grados C (sin offset) */
    uint16_t conteo_eventos;  /* contador de eventos                       */
} payload_t;
```

> El offset `+40` solo existe en la representación **serializada** (byte 7).
> En `payload_t`, `temp_c` es la temperatura real y puede ser negativa.

---

## 4. API del codec (C — `payload_codec.h` / `.c`)

```c
/* Serializa payload_t -> 12 bytes. BE para magnitud_mg y conteo_eventos.
   Bytes 10-11 siempre 0x00.
   Retorna PAYLOAD_OK o PAYLOAD_ERR_NULL. */
int payload_encode(const payload_t *p, uint8_t out[PAYLOAD_LEN]);

/* Deserializa 12 bytes -> payload_t (tests en host y downlink futuro).
   Retorna PAYLOAD_OK, PAYLOAD_ERR_NULL o PAYLOAD_ERR_TIPO. */
int payload_decode(const uint8_t in[PAYLOAD_LEN], payload_t *p);
```

Características de la implementación:

- **C puro, sin HAL.** Solo incluye `<stdint.h>`; compila en host (gcc/clang)
  para generar y validar vectores fuera del firmware.
- `payload_encode` escribe `out[7] = temp_c + PAYLOAD_TEMP_OFFSET` y fuerza
  `out[10] = out[11] = 0x00`.
- `payload_decode` valida el tipo (`in[0]`) contra los dos valores conocidos,
  aplica el offset inverso a la temperatura e **ignora** los bytes reservados.

---

## 5. API del parser (Python — `App/Backend/payload_parser.py`)

Espeja byte a byte el codec C. Debe producir exactamente los mismos campos.

```python
parse_payload(hex_string: str) -> dict
encode_payload(tipo, fuente=0, magnitud_mg=0, eje=0, magnetico=0,
               bateria_pct=0, temp_c=0, conteo_eventos=0) -> str
```

- `parse_payload` tolera espacios / guiones / mayúsculas; exige 24 chars hex;
  lanza `PayloadError` ante longitud inválida, hex inválido o tipo desconocido.
- El `dict` devuelto incluye campos crudos e interpretados legibles:
  `tipo`/`tipo_str`, `fuente`/`fuente_accel`/`fuente_magnetico`,
  `magnitud_mg`, `eje`/`eje_str`, `magnetico`/`magnetico_str`,
  `bateria_pct`, `temp_c`, `conteo_eventos`, `reservado`, `raw_hex`.

Constantes espejo en Python: `PAYLOAD_LEN=12`, `PAYLOAD_HEX_LEN=24`,
`TIPO_ALARMA=0x01`, `TIPO_HEARTBEAT=0x02`, `FUENTE_ACCEL=0x01`,
`FUENTE_MAGNETICO=0x02`, `TEMP_OFFSET=40`.

Tablas de texto: `_TIPO_STR`, `_EJE_STR`, `_MAG_STR`.

---

## 6. Vectores de prueba (validados C ↔ Python)

El hex esperado coincide **byte a byte** entre el codec C y el parser Python.

| # | Vector | Descripción | Hex esperado (12 bytes) |
|---|--------|-------------|--------------------------|
| V1 | Alarma accel | 200 mg, eje Z, bat 90 %, 25 °C, 15 eventos | `010100C803005A41000F0000` |
| V2 | Alarma magnético | puerta abierta, bat 88 %, 22 °C, 16 eventos | `010200000001583E00100000` |
| V3 | Alarma dual | 350 mg eje X + puerta abierta, bat 85 %, 30 °C, 17 eventos | `0103015E0101554600110000` |
| V4 | Heartbeat | keepalive 24 h, bat 75 %, 4 °C, 0 eventos | `0200000000004B2C00000000` |
| V5 | Temp negativa | heartbeat, bat 60 %, **−10 °C** (−10+40=30=`0x1E`), 0 eventos | `0200000000003C1E00000000` |

<details>
<summary>Desglose byte a byte (V1 — Alarma por acelerómetro)</summary>

| Campo | Valor | Byte(s) | Hex |
|-------|-------|---------|-----|
| tipo | alarma | 0 | `01` |
| fuente | accel (`0x01`) | 1 | `01` |
| magnitud_mg | 200 (`0x00C8`) | 2–3 | `00 C8` |
| eje | Z (3) | 4 | `03` |
| magnetico | cerrado (0) | 5 | `00` |
| bateria_pct | 90 | 6 | `5A` |
| temp | 25 °C → 65 | 7 | `41` |
| conteo_eventos | 15 (`0x000F`) | 8–9 | `00 0F` |
| reservado | 0 | 10–11 | `00 00` |

</details>

---

## 7. Cómo reproducir / validar

**C (desde `Tests/payload/`):**

```sh
gcc -std=c99 -Wall -Wextra -I../../Firmware/app/payload \
    ../../Firmware/app/payload/payload_codec.c \
    test_payload_host.c -o test_payload_host && ./test_payload_host
```

Salida esperada: `5/5 vectores OK`.

**Python (desde la raíz del repo):**

```sh
py App/Backend/payload_parser.py      # o python3 según el sistema
```

Decodifica e imprime los mismos 5 vectores.

---

## 8. Estado de validación (WP-01)

- [x] Hex de los 5 vectores coincide entre C y Python.
- [x] Endianness BE verificado (`magnitud_mg`, `conteo_eventos`).
- [x] Offset de temperatura correcto (byte 7 = `temp_c + 40`).
- [x] Compila sin HAL (solo `<stdint.h>`).

---

## 9. Extensión propuesta (v2) — `evento` y tiempo abierto

> **Estado: PROPUESTA, no implementada.** Fuente:
> [`docs/diseno-logico/`](diseno-logico/) — *Diseño lógico del firmware*, v0.1,
> José Francisco Díaz, 23-jul-2026. Nada de esta sección está todavía en
> `payload_codec.{h,c}` ni en `payload_parser.py`.

### 9.1 Por qué

El payload v1 no tiene dónde poner dos cosas que el diseño de la FSM necesita:

1. la **leyenda** de la alarma — `apertura`, `cierre` o `vandalismo`. Hoy el byte 0
   sólo distingue alarma de heartbeat, no *qué clase* de alarma;
2. el **tiempo que la puerta estuvo abierta**.

La propuesta **no cambia la longitud**: reutiliza los dos bytes reservados (10–11)
y refina la semántica de dos campos que ya existen.

### 9.2 Contrato v2

| Byte(s) | Campo | Estado | Definición propuesta |
|---------|-------|--------|----------------------|
| **0** | `tipo` | igual | `0x01` alarma / `0x02` heartbeat |
| **1** | `fuente` | igual | bitfield accel/magnético (qué sensores participaron) |
| **2–3** | `magnitud_mg` | 🔶 **refinado** | `θ_max`: máxima desviación vs `accel_ref` (mg) |
| **4** | `eje` | igual | eje dominante del desplazamiento |
| **5** | `magnetico` | igual | `P`: 0 cerrado / 1 abierto (**estado final** de la ventana) |
| **6** | `bateria_pct` | igual | batería 0..100 |
| **7** | `temp` | igual | `temp_c + 40` |
| **8–9** | `conteo` | 🔶 **redefinido** | `N`: aperturas dentro de la ventana (BE) |
| **10** | `evento` | 🟢 **NUEVO** | `0x00` ninguno · `0x01` apertura · `0x02` cierre · `0x03` vandalismo |
| **11** | `t_abierto_s` | 🟢 **NUEVO** | duración abierta en segundos (0–255, **saturado**) |

Los campos "refinados" no cambian de tipo ni de posición — cambia lo que
*significan*, y eso es justo lo que hay que dejar por escrito para que el backend
no los interprete mal:

- **`magnitud_mg` → `θ_max`.** Antes era "magnitud de aceleración"; ahora es
  específicamente la desviación máxima respecto a `accel_ref`, el vector de
  gravedad capturado **al armar** con la puerta cerrada. Es la variable que
  discrimina apertura real de forcejeo.
- **`conteo` → `N`.** Antes era un contador monotónico de eventos del dispositivo.
  Se libera porque **Sigfox ya entrega un número de secuencia a nivel de red**
  que el backend ve, así que el contador de la app era redundante. Ese byte rinde
  más llevando el conteo de aperturas de la ventana.
- **`magnetico` → `P`.** Mismo campo, pero es el estado **al cierre de la ventana
  de observación**, no el instantáneo del flanco que despertó al MCU.

### 9.3 Constantes nuevas

```c
/* Byte 10: evento / leyenda de la alarma */
typedef enum {
    EV_NINGUNO    = 0x00,
    EV_APERTURA   = 0x01,
    EV_CIERRE     = 0x02,
    EV_VANDALISMO = 0x03
} evento_t;
```

### 9.4 De dónde sale `evento`: la clasificación

Tres booleanas derivadas de las variables de la ventana de observación:

```
C = (N > n_umbral),  n_umbral = 5           "muchos ciclos"
M = (θ_max ≥ θ_umbral)                      "movimiento real"
P = puerta abierta al cierre de la ventana  "estado final"
```

Minimizadas con Karnaugh (los 8 casos cubiertos, mutuamente excluyentes):

```
VANDALISMO: V = C · !M
APERTURA:   A = P · (!C + M)
CIERRE:     Z = !P · (!C + M)
```

```c
evento_t clasificar(uint16_t N, uint16_t th_max, bool puerta_abierta) {
    bool C = (N > N_UMBRAL);            /* muchos ciclos    */
    bool M = (th_max >= THETA_UMBRAL);  /* movimiento real  */
    bool P = puerta_abierta;            /* estado final     */

    if (C && !M)  return EV_VANDALISMO; /* V = C . !M        */
    if (P)        return EV_APERTURA;   /* A = P . (!C + M)  */
    else          return EV_CIERRE;     /* Z = !P . (!C + M) */
}
```

Tabla de verdad completa, mapas de Karnaugh y la FSM: ver el PDF en
[`docs/diseno-logico/`](diseno-logico/).

### 9.5 `t_abierto_s` — se mide con RTC, no con timer

`t0 = RTC` en el flanco de apertura → el MCU **vuelve a Stop2** → al flanco de
cierre, `t_ab = RTC - t0`. Medir cuesta dos lecturas de registro (~3 µA todo el
intervalo). El enfoque con timer de propósito general y el MCU despierto se
**descarta**: son mA en vez de µA, ~670× más consumo.

El byte satura en 255 s (4 min 15 s). En RAM la variable es `uint16_t`; la
saturación ocurre sólo al serializar.

### 9.6 Compatibilidad con los vectores actuales

| Vectores | Efecto de la v2 |
|---|---|
| **V4, V5** (heartbeat) | ✅ **Siguen válidos.** En heartbeat los bytes 10–11 quedan en `0x00`, igual que hoy. |
| **V1, V2, V3** (alarma) | ⚠️ **Hay que recalcularlos** para incluir `evento` (y `t_abierto_s` cuando aplique). |

Un decodificador v1 leyendo un mensaje v2 no truena — ignora los bytes 10–11 —
pero **pierde la leyenda**, que es justo el dato nuevo. Por eso codec, parser y
backend tienen que migrar juntos.

### 9.7 Config asociada (`device_config_t`)

La v2 necesita tres parámetros nuevos en la config de flash:

```c
typedef struct {
    /* --- ya existentes --- */
    uint16_t accel_threshold_mg;  /* 200   -> tambien sirve como THETA_UMBRAL */
    uint16_t cooldown_seconds;    /* 60    -> anti-flood entre TX             */
    int8_t   tx_power_dbm;        /* 14                                       */
    uint8_t  heartbeat_hours;     /* 24                                       */
    uint8_t  daily_msg_limit;     /* 130                                      */
    /* --- NUEVOS para esta logica --- */
    uint16_t t_obs_ms;            /* 10000 -> ventana de observacion          */
    uint8_t  n_umbral;            /* 5     -> umbral de ciclos (vandalismo)   */
    uint8_t  debounce_ms;         /* 30-50 -> antirrebote del Hall/reed       */
} device_config_t;
```

---

## 10. Pendientes para implementar la v2

Checklist para cerrar la migración. **Los cuatro primeros van juntos**: si se
mueve uno sin los otros, C y Python dejan de coincidir y la validación 5/5 se cae.

- [ ] `Firmware/app/payload/payload_codec.h` — agregar `evento_t`, los campos
      `evento` y `t_abierto_s` a `payload_t`, y las constantes `EV_*`.
- [ ] `Firmware/app/payload/payload_codec.c` — escribir bytes 10–11 en
      `payload_encode` (hoy los fuerza a `0x00`) y leerlos en `payload_decode`
      (hoy los ignora); saturar `t_abierto_s` a 255.
- [ ] `App/Backend/payload_parser.py` — espejo: `evento`/`evento_str` y
      `t_abierto_s` en el dict; quitar `reservado` o dejarlo como alias.
- [ ] `Tests/payload/test_vectors.md` + `test_payload_host.c` — **recalcular
      V1–V3**; V4/V5 no cambian. Volver a correr y confirmar 5/5.
- [ ] `Firmware/app/payload/VALIDACION.md` — agregar los checks de los campos nuevos.
- [ ] `docs/spec-producto.md` §4.4 — alinear la tabla del payload con la v2.
- [ ] Backend / callback Sigfox (`docs/sigfox-callback-config.md`) — mapear
      `evento` a la leyenda que ve el usuario final.
- [ ] Calibrar `θ_umbral` y `debounce_ms` con la puerta real antes de congelar
      los defaults.
- [ ] Asignar el **GPIO del botón de armado** (EXTI libre) — sin él la guarda de
      armado no existe y la FSM no arranca.

> Mientras la v2 no esté implementada, **§1–§8 siguen siendo el contrato real**.
> Esta sección es el destino, no el estado.
