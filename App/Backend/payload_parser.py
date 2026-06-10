"""
payload_parser.py — Decoder del payload Sigfox de 12 bytes de 0G LockControl.

Espeja byte a byte el contrato implementado en Firmware/app/payload/payload_codec.c
Debe producir EXACTAMENTE los mismos campos que el codec C para los mismos bytes.

Contrato (12 bytes, big-endian en los campos uint16):
    Byte  0      tipo        (0x01 alarma, 0x02 heartbeat)
    Byte  1      fuente      bitfield (bit0 accel, bit1 magnetico)
    Bytes 2-3    magnitud_mg uint16 BIG-ENDIAN
    Byte  4      eje         (0 ninguno, 1 X, 2 Y, 3 Z)
    Byte  5      magnetico   (0 cerrado, 1 abierto)
    Byte  6      bateria_pct (0..100)
    Byte  7      temp        (temp_c + 40)
    Bytes 8-9    conteo      uint16 BIG-ENDIAN
    Bytes 10-11  reservado   (0x0000)

Uso:
    >>> from payload_parser import parse_payload
    >>> parse_payload("010100C803005A41000F0000")["magnitud_mg"]
    200
"""

from __future__ import annotations

# ---- Constantes (mismas que payload_codec.h) ----
PAYLOAD_LEN = 12
PAYLOAD_HEX_LEN = PAYLOAD_LEN * 2  # 24 caracteres hex

TIPO_ALARMA = 0x01
TIPO_HEARTBEAT = 0x02

FUENTE_ACCEL = 0x01
FUENTE_MAGNETICO = 0x02

TEMP_OFFSET = 40

_TIPO_STR = {TIPO_ALARMA: "alarma", TIPO_HEARTBEAT: "heartbeat"}
_EJE_STR = {0: "ninguno", 1: "X", 2: "Y", 3: "Z"}
_MAG_STR = {0: "cerrado", 1: "abierto"}


class PayloadError(ValueError):
    """Error de validacion o decodificacion del payload."""


def parse_payload(hex_string: str) -> dict:
    """Decodifica un payload Sigfox de 12 bytes desde su representacion hex.

    Args:
        hex_string: 24 caracteres hexadecimales (con o sin espacios / mayusculas).

    Returns:
        dict con los campos ya interpretados.

    Raises:
        PayloadError: si la longitud no es exacta, no es hex valido, o el tipo
                      es desconocido.
    """
    if not isinstance(hex_string, str):
        raise PayloadError("hex_string debe ser str")

    # Tolerar espacios y mayusculas; rechazar cualquier otra cosa.
    cleaned = hex_string.strip().replace(" ", "").replace("-", "").lower()

    if len(cleaned) != PAYLOAD_HEX_LEN:
        raise PayloadError(
            f"longitud invalida: se esperaban {PAYLOAD_HEX_LEN} chars hex, "
            f"se recibieron {len(cleaned)}"
        )

    try:
        b = bytes.fromhex(cleaned)
    except ValueError as exc:
        raise PayloadError(f"hex invalido: {exc}") from exc

    tipo = b[0]
    if tipo not in (TIPO_ALARMA, TIPO_HEARTBEAT):
        raise PayloadError(f"tipo desconocido: 0x{tipo:02X}")

    fuente = b[1]
    magnitud_mg = int.from_bytes(b[2:4], "big")   # BIG-ENDIAN
    eje = b[4]
    magnetico = b[5]
    bateria_pct = b[6]
    temp_c = b[7] - TEMP_OFFSET                    # offset inverso (+40)
    conteo_eventos = int.from_bytes(b[8:10], "big")  # BIG-ENDIAN
    reservado = int.from_bytes(b[10:12], "big")

    return {
        "tipo": tipo,
        "tipo_str": _TIPO_STR.get(tipo, "desconocido"),
        "fuente": fuente,
        "fuente_accel": bool(fuente & FUENTE_ACCEL),
        "fuente_magnetico": bool(fuente & FUENTE_MAGNETICO),
        "magnitud_mg": magnitud_mg,
        "eje": eje,
        "eje_str": _EJE_STR.get(eje, "desconocido"),
        "magnetico": magnetico,
        "magnetico_str": _MAG_STR.get(magnetico, "desconocido"),
        "bateria_pct": bateria_pct,
        "temp_c": temp_c,
        "conteo_eventos": conteo_eventos,
        "reservado": reservado,
        "raw_hex": cleaned,
    }


def encode_payload(
    tipo: int,
    fuente: int = 0,
    magnitud_mg: int = 0,
    eje: int = 0,
    magnetico: int = 0,
    bateria_pct: int = 0,
    temp_c: int = 0,
    conteo_eventos: int = 0,
) -> str:
    """Construye el hex de 12 bytes a partir de campos.

    Util para generar vectores de prueba y comparar contra el codec C.
    """
    b = bytearray(PAYLOAD_LEN)
    b[0] = tipo & 0xFF
    b[1] = fuente & 0xFF
    b[2:4] = int(magnitud_mg & 0xFFFF).to_bytes(2, "big")
    b[4] = eje & 0xFF
    b[5] = magnetico & 0xFF
    b[6] = bateria_pct & 0xFF
    b[7] = (temp_c + TEMP_OFFSET) & 0xFF
    b[8:10] = int(conteo_eventos & 0xFFFF).to_bytes(2, "big")
    b[10] = 0x00
    b[11] = 0x00
    return b.hex().upper()


if __name__ == "__main__":
    # Demo rapida con los 5 vectores compartidos (ver Tests/payload/test_vectors.md)
    vectores = {
        "alarma_accel":    "010100C803005A41000F0000",
        "alarma_magnetico":"010200000001583E00100000",
        "alarma_dual":     "0103015E0101554600110000",
        "heartbeat":       "0200000000004B2C00000000",
        "temp_negativa":   "0200000000003C1E00000000",
    }
    for nombre, hx in vectores.items():
        p = parse_payload(hx)
        print(f"{nombre:18s} -> tipo={p['tipo_str']:9s} "
              f"mag={p['magnitud_mg']:4d}mg eje={p['eje_str']:7s} "
              f"mag_sensor={p['magnetico_str']:8s} bat={p['bateria_pct']:3d}% "
              f"temp={p['temp_c']:4d}C conteo={p['conteo_eventos']}")
