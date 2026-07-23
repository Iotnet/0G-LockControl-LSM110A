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

---

## 1. Contrato del payload

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
