# Configuración del backend Sigfox — Custom payload + Callback uplink

Configuración para el Device Type de **0G LockControl** en
[backend.sigfox.com](https://backend.sigfox.com). Cubre (1) el *custom payload
display*, (2) el *callback UPLINK* tipo URL, (3) la interpretación de cada campo
y (4) los pasos exactos en la consola.

> Contrato del payload: 12 bytes. Ver `Firmware/app/payload/payload_codec.h` y
> `Tests/payload/test_vectors.md`. Los `uint:16` son **big-endian**, que es el
> valor por defecto del grammar de Sigfox (no hace falta sufijo).

---

## 1. Custom payload config (Display type → Custom)

En **Device Type → Edit → Display type = Custom**, pegar en *Custom grammar*:

```
tipo::uint:8 fuente::uint:8 magnitud::uint:16 eje::uint:8 magnetico::uint:8 bateria::uint:8 temp::uint:8 conteo::uint:16
```

### Por qué esta sintaxis es correcta

Gramática Sigfox: `nombre : byteIndex : tipo`.

- **`::` (doble dos-puntos)** = índice de byte automático: cada campo arranca
  donde terminó el anterior. Con esto no hay que contar offsets a mano.
- **`uint:N`** = entero sin signo de N bits. Endianness **big-endian por
  defecto** (posición de bit 7) → coincide con nuestros `uint16` BE. No se
  añade `:little-endian`.
- Los **bytes 10–11 (reservado)** se omiten: el grammar no necesita mapear
  todos los bytes.

Posicionamiento que resulta (verificar contra el contrato):

| Campo grammar | Bytes | Tipo |
|---------------|-------|------|
| `tipo` | 0 | uint:8 |
| `fuente` | 1 | uint:8 |
| `magnitud` | 2–3 | uint:16 BE |
| `eje` | 4 | uint:8 |
| `magnetico` | 5 | uint:8 |
| `bateria` | 6 | uint:8 |
| `temp` | 7 | uint:8 (raw, ver nota) |
| `conteo` | 8–9 | uint:16 BE |
| (reservado) | 10–11 | no mapeado |

> **Nota temp:** el grammar de Sigfox no aplica offsets aritméticos. El campo
> `temp` muestra el **byte crudo (0–255)**. La conversión a °C real
> (`temp_real = temp - 40`) se hace en el backend (ver `payload_parser.py`).

---

## 2. Callback UPLINK (tipo URL)

En **Device Type → Callbacks → New → Custom callback**:

| Campo | Valor |
|-------|-------|
| Type | `DATA` / `UPLINK` |
| Channel | `URL` |
| URL pattern | `https://<TU_ENDPOINT>/sigfox/uplink` |
| Use HTTP Method | `POST` |
| Content-Type | `application/json` |
| Send SNI | ✔ (si el endpoint usa HTTPS con SNI) |

### Body template (variables Sigfox)

```json
{
  "device": "{device}",
  "time": {time},
  "data": "{data}",
  "seqNumber": {seqNumber},
  "lqi": "{lqi}"
}
```

- `{device}` — ID del dispositivo (hex). **String** → entre comillas.
- `{time}` — epoch en segundos. **Número** → sin comillas.
- `{data}` — payload en hex (24 chars = 12 bytes). **String**.
- `{seqNumber}` — número de secuencia del mensaje. **Número**.
- `{lqi}` — link quality indicator. Se deja como **string**.

### Ejemplo de JSON que recibirá el endpoint

(usando el vector V1 — alarma accel, ver `test_vectors.md`)

```json
{
  "device": "1A2B3C",
  "time": 1749513600,
  "data": "010100C803005A41000F0000",
  "seqNumber": 42,
  "lqi": "Good"
}
```

---

## 3. Interpretación de campos tras el parse

| Campo | Origen | Unidad / valores | Rango válido |
|-------|--------|------------------|--------------|
| `tipo` | byte 0 | `0x01` alarma · `0x02` heartbeat | {1, 2} |
| `fuente` | byte 1 | bitfield: bit0 accel, bit1 magnético | 0–3 |
| `magnitud` | bytes 2–3 | mg (miligravedades) | 0–65535 |
| `eje` | byte 4 | 0 ninguno · 1 X · 2 Y · 3 Z | 0–3 |
| `magnetico` | byte 5 | 0 cerrado · 1 abierto | 0–1 |
| `bateria` | byte 6 | % | 0–100 |
| `temp` | byte 7 | **°C real = `temp - 40`** | byte 0–255 → −40…+215 °C |
| `conteo` | bytes 8–9 | conteo de eventos | 0–65535 |

> Margen de duty-cycle Sigfox: 140 msg/día máximo. El firmware limita a 130
> (ver `Firmware/app/alarm_logic/`), dejando 10 para heartbeats.

---

## 4. Pasos en backend.sigfox.com

1. **Login** en [backend.sigfox.com](https://backend.sigfox.com) con la cuenta
   del grupo (UnaBiz / WND Mexico, RC2).
2. **Device Type → [el del proyecto] → Edit.**
3. En **Display type** elegir **Custom** y pegar el grammar de la sección 1.
   Guardar.
4. Ir a **Callbacks → New.**
5. Elegir **Custom callback**, tipo **DATA / UPLINK**, canal **URL**.
6. Rellenar URL, método **POST**, Content-Type **application/json** y el **Body**
   de la sección 2.
7. Guardar. Usar el botón **Download / Test** o enviar un uplink real y
   verificar en el endpoint (`/sigfox/uplink` del backend-mock, WP-04).
8. Para depurar: **Device → Messages** muestra los uplinks crudos y el resultado
   del custom display.

---

## Referencias

- Sigfox — *Payload display parameter* (custom grammar):
  https://support.sigfox.com/docs/payload-display-parameter
- Sigfox — *Callback creation* (variables `{device}`, `{time}`, `{data}`,
  `{seqNumber}`, `{lqi}`).
