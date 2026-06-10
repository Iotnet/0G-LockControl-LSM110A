# VALIDACIÓN — WP-01 Codec del payload

Checklist para revisión rápida (Franco).

## Checklist

- [x] **Hex de los 5 vectores coincide entre C y Python**
  - C: `5/5 vectores OK` (ver salida abajo)
  - Python: `py App/Backend/payload_parser.py` decodifica los mismos 5 hex
- [x] **Endianness BE verificado** — `magnitud_mg` (bytes 2–3) y `conteo_eventos`
  (bytes 8–9) se serializan MSB primero. V3: 350 → `01 5E`.
- [x] **Offset de temperatura correcto** — byte 7 = `temp_c + 40`.
  V5: −10 °C → `0x1E` (30). V4: 4 °C → `0x2C` (44).
- [x] **Compila sin HAL** — `payload_codec.c` solo incluye `payload_codec.h`
  (que solo usa `<stdint.h>`). Compila con gcc en host, sin `stm32wlxx_hal.h`.

## Cómo reproducir

**C (desde `Tests/payload/`):**
```
gcc -std=c99 -Wall -Wextra -I../../Firmware/app/payload \
    ../../Firmware/app/payload/payload_codec.c \
    test_payload_host.c -o test_payload_host && ./test_payload_host
```
Salida esperada:
```
[PASS] V1 alarma accel        010100C803005A41000F0000
[PASS] V2 alarma magnetico    010200000001583E00100000
[PASS] V3 alarma dual         0103015E0101554600110000
[PASS] V4 heartbeat           0200000000004B2C00000000
[PASS] V5 temp negativa       0200000000003C1E00000000

5/5 vectores OK
```

**Python (desde la raíz del repo):**
```
py App/Backend/payload_parser.py
```
(o `python3` según el sistema). Decodifica los mismos 5 vectores.

## Archivos del WP-01

| Archivo | Rol |
|---------|-----|
| `Firmware/app/payload/payload_codec.h` | declaraciones + contrato de bytes |
| `Firmware/app/payload/payload_codec.c` | encode/decode, C puro sin HAL |
| `App/Backend/payload_parser.py` | parser Python (`parse_payload`) + `encode_payload` |
| `Tests/payload/test_vectors.md` | 5 vectores con tablas de campos + hex |
| `Tests/payload/test_payload_host.c` | test host C (encode + round-trip) |

> **Nota de ubicación:** el parser Python está en `App/Backend/` (B mayúscula),
> que es el directorio ya versionado en el repo. El prompt original decía
> `App/backend/`; se ajustó el casing para no crear una colisión de
> mayúsculas/minúsculas en git sobre Windows.
