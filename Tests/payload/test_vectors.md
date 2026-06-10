# Vectores de prueba — Payload Sigfox 12 bytes

Vectores compartidos entre el codec C (`Firmware/app/payload/payload_codec.c`) y el
parser Python (`App/Backend/payload_parser.py`). **El hex esperado debe coincidir
byte a byte entre ambas implementaciones.**

## Contrato de bytes (recordatorio)

| Byte | Campo | Notas |
|------|-------|-------|
| 0 | `tipo` | `0x01` alarma · `0x02` heartbeat |
| 1 | `fuente` | bitfield: bit0 `0x01` accel · bit1 `0x02` magnético |
| 2–3 | `magnitud_mg` | uint16 **BIG-ENDIAN** |
| 4 | `eje` | 0 ninguno · 1 X · 2 Y · 3 Z |
| 5 | `magnetico` | 0 cerrado · 1 abierto |
| 6 | `bateria_pct` | 0..100 |
| 7 | `temp` | `temp_c + 40` (offset para negativos) |
| 8–9 | `conteo_eventos` | uint16 **BIG-ENDIAN** |
| 10–11 | reservado | siempre `0x0000` |

---

## V1 — Alarma por acelerómetro

200 mg, eje Z, batería 90 %, 25 °C, 15 eventos.

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

**Hex esperado:** `010100C803005A41000F0000`

---

## V2 — Alarma por magnético (puerta abierta)

Sin magnitud de accel, puerta abierta, batería 88 %, 22 °C, 16 eventos.

| Campo | Valor | Byte(s) | Hex |
|-------|-------|---------|-----|
| tipo | alarma | 0 | `01` |
| fuente | magnético (`0x02`) | 1 | `02` |
| magnitud_mg | 0 | 2–3 | `00 00` |
| eje | ninguno (0) | 4 | `00` |
| magnetico | abierto (1) | 5 | `01` |
| bateria_pct | 88 | 6 | `58` |
| temp | 22 °C → 62 | 7 | `3E` |
| conteo_eventos | 16 (`0x0010`) | 8–9 | `00 10` |
| reservado | 0 | 10–11 | `00 00` |

**Hex esperado:** `010200000001583E00100000`

---

## V3 — Alarma dual (ambas fuentes)

350 mg eje X **y** puerta abierta, batería 85 %, 30 °C, 17 eventos.

| Campo | Valor | Byte(s) | Hex |
|-------|-------|---------|-----|
| tipo | alarma | 0 | `01` |
| fuente | accel \| magnético (`0x03`) | 1 | `03` |
| magnitud_mg | 350 (`0x015E`) | 2–3 | `01 5E` |
| eje | X (1) | 4 | `01` |
| magnetico | abierto (1) | 5 | `01` |
| bateria_pct | 85 | 6 | `55` |
| temp | 30 °C → 70 | 7 | `46` |
| conteo_eventos | 17 (`0x0011`) | 8–9 | `00 11` |
| reservado | 0 | 10–11 | `00 00` |

**Hex esperado:** `0103015E0101554600110000`

---

## V4 — Heartbeat (cuarto frío)

Keepalive de 24 h: batería 75 %, 4 °C, 0 eventos.

| Campo | Valor | Byte(s) | Hex |
|-------|-------|---------|-----|
| tipo | heartbeat | 0 | `02` |
| fuente | ninguna (0) | 1 | `00` |
| magnitud_mg | 0 | 2–3 | `00 00` |
| eje | ninguno (0) | 4 | `00` |
| magnetico | cerrado (0) | 5 | `00` |
| bateria_pct | 75 | 6 | `4B` |
| temp | 4 °C → 44 | 7 | `2C` |
| conteo_eventos | 0 | 8–9 | `00 00` |
| reservado | 0 | 10–11 | `00 00` |

**Hex esperado:** `0200000000004B2C00000000`

---

## V5 — Edge case: temperatura negativa

Heartbeat en frío extremo: batería 60 %, **−10 °C**, 0 eventos.
Verifica el offset: `−10 + 40 = 30 = 0x1E`.

| Campo | Valor | Byte(s) | Hex |
|-------|-------|---------|-----|
| tipo | heartbeat | 0 | `02` |
| fuente | ninguna (0) | 1 | `00` |
| magnitud_mg | 0 | 2–3 | `00 00` |
| eje | ninguno (0) | 4 | `00` |
| magnetico | cerrado (0) | 5 | `00` |
| bateria_pct | 60 | 6 | `3C` |
| temp | −10 °C → **30** | 7 | `1E` |
| conteo_eventos | 0 | 8–9 | `00 00` |
| reservado | 0 | 10–11 | `00 00` |

**Hex esperado:** `0200000000003C1E00000000`

---

## Resumen (para diff rápido C ↔ Python)

| # | Vector | Hex esperado (12 bytes) |
|---|--------|--------------------------|
| V1 | Alarma accel | `010100C803005A41000F0000` |
| V2 | Alarma magnético | `010200000001583E00100000` |
| V3 | Alarma dual | `0103015E0101554600110000` |
| V4 | Heartbeat | `0200000000004B2C00000000` |
| V5 | Temp negativa | `0200000000003C1E00000000` |

> Validación Python: `py App/Backend/payload_parser.py` imprime los 5 decodificados.
> Validación C: ver `Tests/payload/test_payload_host.c`.
