# Definiciones del Payload — 0G LockControl (LSM110A)

Documento de referencia del **payload Sigfox** del proyecto (WP-01 "Codec del
payload"). Reúne el contrato de bytes, el análisis que lo justifica, las
constantes, la estructura en memoria, la API de codec (C), el parser (Python) y
los vectores de prueba.

> **Fuentes en el repo** (esto es un resumen; el código es la verdad):
> - `Firmware/app/payload/payload_codec.h` — declaraciones + contrato
> - `Firmware/app/payload/payload_codec.c` — encode/decode, C puro sin HAL
> - `App/Backend/payload_parser.py` — parser Python (`parse_payload` / `encode_payload`)
> - `Tests/payload/test_vectors.md` — 5 vectores con tablas y hex
> - `Tests/payload/test_payload_host.c` — test host en C
> - `Firmware/app/payload/VALIDACION.md` — checklist de validación
> - [`docs/diseno-logico/`](diseno-logico/) — **diseño lógico del firmware
>   (FSM + clasificación de eventos)**, de donde sale el contrato de §1

---

## ⚠️ Estado de implementación — leer antes de usar este documento

| Versión | Qué es | Estado | Dónde |
|---|---|---|---|
| **v2** | Contrato **diseñado**. Sale del análisis de la FSM: leyenda del evento + tiempo abierto. | 🔶 **Diseño aprobado, NO implementado** | §1 · §2 · §3 |
| **v1** | Contrato **histórico**, asignado antes del análisis. | ✅ **Implementado y validado 5/5** | §4 |

**Este documento lidera con la v2 porque es el contrato analizado** — FSM, tabla
de verdad, Karnaugh y presupuesto energético lo respaldan. La v1 se asignó antes
de ese análisis y se conserva en §4 porque **es lo que el firmware transmite
hoy**.

> 🔻 **Si estás decodificando un uplink real en este momento, usa la tabla de
> §4.1, no la de §1.** Con el firmware actual los bytes 10–11 llegan siempre en
> `0x00` y `conteo` es un contador monotónico, no `N`.

Ambas versiones son de **12 bytes**. La v2 no alarga el payload: ocupa los dos
bytes que la v1 dejó reservados y refina la semántica de dos campos existentes.

---

## 1. Contrato del payload (v2 — diseño)

- **Longitud fija:** 12 bytes (`PAYLOAD_LEN = 12`).
- **Endianness:** los campos `uint16` se serializan en **big-endian** (MSB primero).
- **Transporte:** Sigfox (payload de usuario de 12 bytes).
- **Origen:** [`docs/diseno-logico/`](diseno-logico/) — *Diseño lógico del
  firmware*, v0.1, José Francisco Díaz, 23-jul-2026.

| Byte(s) | Campo | Tipo | Estado | Definición |
|---------|-------|------|--------|------------|
| **0** | `tipo` | `uint8` | igual | `0x01` alarma · `0x02` heartbeat |
| **1** | `fuente` | `uint8` (bitfield) | igual | bit0 = acelerómetro (`0x01`) · bit1 = magnético (`0x02`); combinables con OR. Qué sensores participaron. |
| **2–3** | `magnitud_mg` | `uint16` **BE** | 🔶 refinado | `θ_max`: máxima desviación vs `accel_ref` durante la ventana (mg) |
| **4** | `eje` | `uint8` | igual | `0` ninguno · `1` X · `2` Y · `3` Z — eje dominante del desplazamiento |
| **5** | `magnetico` | `uint8` | 🔶 refinado | `P`: `0` cerrado / `1` abierto — **estado al cierre de la ventana** |
| **6** | `bateria_pct` | `uint8` | igual | Nivel de batería 0..100 |
| **7** | `temp` | `uint8` | igual | `temp_c + 40` (offset para soportar negativos) |
| **8–9** | `conteo` | `uint16` **BE** | 🔶 redefinido | `N`: aperturas dentro de la ventana de observación |
| **10** | `evento` | `uint8` | 🟢 **NUEVO** | `0x00` ninguno · `0x01` apertura · `0x02` cierre · `0x03` vandalismo |
| **11** | `t_abierto_s` | `uint8` | 🟢 **NUEVO** | Duración abierta en segundos (0–255, **saturado**) |

**Nota sobre temperatura:** el byte 7 lleva el offset `+40`; la temperatura real
`temp_c` puede ser negativa. Con `int8_t` y offset 40 el rango representable es
**−40 .. +87 °C**.

### Qué significa "refinado" / "redefinido"

Estos campos **no cambian de tipo ni de posición** — cambia lo que *significan*,
y por eso hay que dejarlo por escrito: el backend los interpretaría mal si asume
la semántica v1.

- **`magnitud_mg` → `θ_max`.** Antes era "magnitud de aceleración" en genérico;
  ahora es específicamente la desviación máxima respecto a `accel_ref`, el vector
  de gravedad capturado **al armar** con la puerta cerrada. Es la variable que
  discrimina apertura real de forcejeo.
- **`conteo` → `N`.** Antes era un contador monotónico de eventos del dispositivo;
  ahora es el número de aperturas **dentro de la ventana**. Ver §2.3.
- **`magnetico` → `P`.** Mismo campo, pero es el estado **al cierre de la ventana
  de observación**, no el instantáneo del flanco que despertó al MCU.

---

## 2. Fundamento del diseño

Lo que justifica cada campo nuevo. El detalle completo —tabla de verdad, mapas de
Karnaugh, FSM, diagramas— está en el PDF de [`docs/diseno-logico/`](diseno-logico/).

### 2.1 De dónde sale `evento`: la clasificación

Tres booleanas derivadas de las variables de la ventana de observación
(`T_OBS` = 10 s):

```
C = (N > n_umbral),  n_umbral = 5           "muchos ciclos"
M = (θ_max ≥ θ_umbral)                      "movimiento real"
P = puerta abierta al cierre de la ventana  "estado final"
```

Minimizadas con Karnaugh (los 8 casos cubiertos, mutuamente excluyentes, sin
huecos ni solapes):

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

El criterio de **vandalismo**: muchos flancos del Hall *sin* que el acelerómetro
confirme que la puerta se abrió de verdad — alguien forcejeó la puerta cerrada,
el reed castañeteó pero la hoja no giró.

### 2.2 `t_abierto_s` se mide con RTC, no con timer

`t0 = RTC` en el flanco de apertura → el MCU **vuelve a Stop2** → al flanco de
cierre, `t_ab = RTC - t0`. Medir cuesta dos lecturas de registro.

| Puerta abierta | RTC + Stop2 (~3 µA) | Timer GP + Run (~2 mA) | Factor |
|---|---|---|---|
| 1 minuto | ~0.00005 mAh | ~0.033 mAh (≈1 TX) | ~670× |
| 1 hora | ~0.003 mAh | ~2 mAh (≈72 TX) | ~670× |
| 8 horas | ~0.024 mAh | ~16 mAh (≈576 TX) | ~670× |

El enfoque con timer de propósito general y el MCU despierto se **descarta**: son
mA en vez de µA. En RAM la variable es `uint16_t`; la saturación a 255 ocurre
sólo al serializar el byte 11.

### 2.3 Por qué se liberó `conteo`

**Sigfox ya entrega un número de secuencia a nivel de red** que el backend ve en
cada mensaje, así que un contador monotónico en la aplicación era redundante. Ese
byte rinde más llevando el conteo de aperturas de la ventana, que es lo que
alimenta la booleana `C` de la clasificación.

> ⚠️ **Regresión menor que esto introduce:** la v1 acumulaba eventos suprimidos
> por el cooldown y los enviaba en la siguiente transmisión
> (`app_sensors.c`: *"durante el cooldown solo se suprime el TX; el conteo sigue
> acumulando"*). Con `N` = aperturas-en-la-ventana, esa señal se pierde: ya no se
> sabe cuántos eventos se descartaron por cooldown o por el tope diario.
> Mitigable con un nibble de "eventos suprimidos" si se considera necesario.
> **Decisión pendiente.**

### 2.4 Ocupación de bits — hallazgo abierto

La asignación v2 es **byte-alineada**, y eso es deliberado y correcto: hay un
codec en C y un parser en Python que deben coincidir byte a byte, y meter máscaras
y corrimientos duplicaría la superficie para bugs de endianness/máscara. Pero al
revisar cuántos bits usa realmente cada campo aparece un desbalance:

| Byte(s) | Campo | Valores reales | Bits que usa | Bits que sobran |
|---|---|---|---|---|
| 0 | `tipo` | 2 | 1–2 | ~6 |
| 1 | `fuente` | bitfield de 2 bits | 2 | 6 |
| 2–3 | `θ_max` | 0–16000 mg (FS ±16 g) | 14 | 2 ✅ justificado |
| 4 | `eje` | 4 | 2 | 6 |
| 5 | `P` | 2 | 1 | 7 |
| 6 | `bateria_pct` | 0–100 | 7 | 1 ✅ |
| 7 | `temp` | −40..+87 | 8 | 0 ✅ |
| 8–9 | **`N`** | **umbral de decisión = 5** | **~8** | **8 ← un byte entero** |
| 10 | `evento` | 4 | 2 | 6 |
| 11 | **`t_abierto_s`** | **0–255 s** | **8** | **0 ← el único apretado** |

**El hallazgo:** `N` recibe 16 bits para contar aperturas cuyo umbral de decisión
es **5**. Con el antirrebote de 30–50 ms, en una ventana de 10 s caben como máximo
~150 flancos físicos — `uint8` sobra. Mientras tanto **`t_abierto_s` es el único
campo que aprieta: satura en 255 s = 4 min 15 s.**

Consecuencia práctica: una puerta abierta 10 minutos, una hora o toda la noche
reportan todas `255`, indistinguibles. Y el caso futuro *"puerta abierta
prolongada"* que contempla el propio documento de diseño sí se puede **detectar**
(en RAM la variable es `uint16_t`) pero **no se puede comunicar** — el mensaje no
puede decir cuánto tiempo fue.

Corrección posible sin cambiar la longitud ni introducir bit-packing:

```
v2 actual                          →   alternativa
8–9   N            uint16 BE           8     N             uint8   (satura 255)
10    evento       uint8               9     evento        uint8
11    t_abierto_s  uint8  (4m15s)      10–11 t_abierto_s   uint16 BE (18.2 h)
```

`t_abierto_s` pasaría de **4 min 15 s a 18.2 horas** de rango, cubriendo el caso
real "se quedó abierta toda la noche", a costo cero en bytes y manteniendo
resolución de 1 segundo.

> **Estado: hallazgo documentado, layout SIN cambiar.** §1 conserva la asignación
> tal como la propone el documento de diseño. Esta corrección queda a decisión de
> José Francisco / Franco. **Conviene resolverla antes de implementar la v2**: una
> vez que el codec, el parser, el backend y los vectores se construyan contra
> §1, moverla cuesta cuatro archivos más recalcular vectores otra vez.

### 2.5 Otra cuestión abierta: `fuente` vs `evento`

`fuente` (byte 1) y `evento` (byte 10) se solapan parcialmente en significado:
el primero dice qué sensores participaron, el segundo el veredicto. Siguen siendo
distintos y útiles (accel+hall vs sólo hall), pero vale revisar si el backend
necesita ambos o si `fuente` se vuelve redundante una vez que existe `evento`.

---

## 3. Constantes y estructuras

### 3.1 Constantes vigentes (`payload_codec.h`)

Ya implementadas; la v2 no las cambia.

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

### 3.2 Constantes nuevas de la v2

🔶 **Por implementar.**

```c
/* Byte 10: evento / leyenda de la alarma */
typedef enum {
    EV_NINGUNO    = 0x00,
    EV_APERTURA   = 0x01,
    EV_CIERRE     = 0x02,
    EV_VANDALISMO = 0x03
} evento_t;
```

### 3.3 Estructura en memoria (`payload_t`)

Representación con los campos **ya interpretados** (sin offset, valores reales).
Lo que hay hoy:

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

🔶 La v2 le agrega dos campos:

```c
    evento_t evento;          /* EV_* — leyenda de la alarma               */
    uint16_t t_abierto_s;     /* duración abierta (s); satura a 255 al serializar */
```

> El offset `+40` solo existe en la representación **serializada** (byte 7).
> En `payload_t`, `temp_c` es la temperatura real y puede ser negativa.

### 3.4 Config asociada (`device_config_t`)

🔶 La v2 necesita tres parámetros nuevos en la config de flash:

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

> El **antirrebote del Hall** (`debounce_ms`) no es opcional: un reed switch
> rebota, y sin filtrar, *una* apertura real podría contarse como varios flancos
> y disparar un "vandalismo" falso. Es la salvaguarda que hace confiable a `N`.

---

## 4. Estado actual — lo que el código hace hoy (v1)

✅ **Implementado y validado 5/5** en `payload_codec.{h,c}` y `payload_parser.py`.
Ésta es la tabla que corresponde a los mensajes que el dispositivo transmite
ahora mismo.

### 4.1 Contrato v1 (para decodificar uplinks reales)

| Byte(s) | Campo | Tipo | Definición |
|---------|-------|------|------------|
| **0** | `tipo` | `uint8` | `0x01` alarma · `0x02` heartbeat |
| **1** | `fuente` | `uint8` (bitfield) | bit0 = acelerómetro (`0x01`) · bit1 = magnético (`0x02`); combinables con OR |
| **2–3** | `magnitud_mg` | `uint16` **BE** | Magnitud de aceleración en mg |
| **4** | `eje` | `uint8` | `0` ninguno · `1` X · `2` Y · `3` Z |
| **5** | `magnetico` | `uint8` | `0` cerrado · `1` abierto |
| **6** | `bateria_pct` | `uint8` | Nivel de batería 0..100 |
| **7** | `temp` | `uint8` | `temp_c + 40` (offset para soportar negativos) |
| **8–9** | `conteo` | `uint16` **BE** | Contador de eventos (monotónico) |
| **10–11** | `reservado` | — | Siempre `0x0000` (se ignora al decodificar) |

### 4.2 Diferencias v1 → v2

| Byte(s) | v1 (hoy) | v2 (diseño) | Tipo de cambio |
|---|---|---|---|
| 2–3 | `magnitud_mg` genérico | `θ_max` vs `accel_ref` | Semántico — mismo tipo y posición |
| 5 | estado instantáneo | `P` al cierre de la ventana | Semántico |
| 8–9 | contador monotónico | `N` aperturas en la ventana | Semántico — cambia por completo el significado |
| 10 | `reservado` = `0x00` | `evento` (leyenda) | **Campo nuevo** |
| 11 | `reservado` = `0x00` | `t_abierto_s` | **Campo nuevo** |

Un decodificador v1 leyendo un mensaje v2 **no truena** — ignora los bytes 10–11 —
pero **pierde la leyenda**, que es justo el dato nuevo. Y leería `conteo` como si
fuera monotónico cuando es `N`. Por eso codec, parser, vectores y backend tienen
que migrar **juntos**.

---

## 5. API del codec (C — `payload_codec.h` / `.c`)

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

🔶 **Para la v2:** `payload_encode` debe dejar de forzar `out[10]`/`out[11]` a
cero y escribir `evento` y `t_abierto_s` (saturando este último a 255);
`payload_decode` debe leerlos en vez de ignorarlos.

---

## 6. API del parser (Python — `App/Backend/payload_parser.py`)

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

🔶 **Para la v2:** agregar `evento`/`evento_str` y `t_abierto_s` al dict, una
tabla `_EVENTO_STR`, y los parámetros correspondientes en `encode_payload`.
`reservado` desaparece o queda como alias.

---

## 7. Vectores de prueba y validación

### 7.1 Vectores v1 (validados C ↔ Python)

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

### 7.2 Cómo reproducir / validar

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

### 7.3 Estado de validación (WP-01)

- [x] Hex de los 5 vectores coincide entre C y Python.
- [x] Endianness BE verificado (`magnitud_mg`, `conteo_eventos`).
- [x] Offset de temperatura correcto (byte 7 = `temp_c + 40`).
- [x] Compila sin HAL (solo `<stdint.h>`).

### 7.4 Impacto de la v2 en los vectores

| Vectores | Efecto de la v2 |
|---|---|
| **V4, V5** (heartbeat) | ✅ **Siguen válidos.** En heartbeat los bytes 10–11 quedan en `0x00`, igual que hoy. |
| **V1, V2, V3** (alarma) | ⚠️ **Hay que recalcularlos** para incluir `evento` (y `t_abierto_s` cuando aplique). |

La v2 no tiene todavía **ningún** vector de prueba. Hasta que los tenga y la
validación vuelva a dar 5/5, §4 sigue siendo el contrato real.

---

## 8. Checklist de migración a la v2

**Los cuatro primeros van juntos**: si se mueve uno sin los otros, C y Python
dejan de coincidir y la validación 5/5 se cae.

- [ ] **Resolver primero** el hallazgo de §2.4 (`t_abierto_s` de 1 vs 2 bytes) y
      la cuestión abierta de §2.3 (eventos suprimidos). Decidirlo *antes* de
      escribir código evita rehacer los cuatro puntos siguientes.
- [ ] `Firmware/app/payload/payload_codec.h` — agregar `evento_t`, los campos
      `evento` y `t_abierto_s` a `payload_t`, y las constantes `EV_*`.
- [ ] `Firmware/app/payload/payload_codec.c` — escribir bytes 10–11 en
      `payload_encode` (hoy los fuerza a `0x00`) y leerlos en `payload_decode`
      (hoy los ignora); saturar `t_abierto_s`.
- [ ] `App/Backend/payload_parser.py` — espejo: `evento`/`evento_str` y
      `t_abierto_s` en el dict; quitar `reservado` o dejarlo como alias.
- [ ] `Tests/payload/test_vectors.md` + `test_payload_host.c` — **recalcular
      V1–V3**; V4/V5 no cambian. Volver a correr y confirmar 5/5.
- [ ] `Firmware/app/payload/VALIDACION.md` — agregar los checks de los campos nuevos.
- [ ] `docs/spec-producto.md` §4.4 — alinear la tabla del payload con la v2.
- [ ] Backend / callback Sigfox (`docs/sigfox-callback-config.md`) — mapear
      `evento` a la leyenda que ve el usuario final.
- [ ] `Firmware/app/app_sensors.c` — poblar los campos nuevos al armar el payload
      (hoy sólo llena los de la v1).
- [ ] Calibrar `θ_umbral` y `debounce_ms` con la puerta real antes de congelar
      los defaults.
- [ ] Asignar el **GPIO del botón de armado** (EXTI libre) — sin él la guarda de
      armado no existe y la FSM no arranca.

> Mientras estos puntos no estén cerrados, **§4 es el contrato real** y §1 es el
> destino.
