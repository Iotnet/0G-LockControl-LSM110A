/**
  ******************************************************************************
  * @file    payload_codec.c
  * @brief   Implementacion del codec del payload Sigfox de 12 bytes
  * @author  0G IoT Solutions
  *
  * @note    C puro, SIN HAL. Ver payload_codec.h para el contrato de bytes.
  ******************************************************************************
  */

#include "payload_codec.h"

int payload_encode(const payload_t *p, uint8_t out[PAYLOAD_LEN])
{
    if (p == 0 || out == 0) {
        return PAYLOAD_ERR_NULL;
    }

    /* Byte 0: tipo */
    out[0] = p->tipo;

    /* Byte 1: fuente (bitfield) */
    out[1] = p->fuente;

    /* Bytes 2-3: magnitud_mg en BIG-ENDIAN (MSB primero) */
    out[2] = (uint8_t)((p->magnitud_mg >> 8) & 0xFF);
    out[3] = (uint8_t)(p->magnitud_mg & 0xFF);

    /* Byte 4: eje */
    out[4] = p->eje;

    /* Byte 5: estado magnetico */
    out[5] = p->magnetico;

    /* Byte 6: bateria % */
    out[6] = p->bateria_pct;

    /* Byte 7: temperatura con offset (+40) para soportar negativos */
    out[7] = (uint8_t)(p->temp_c + PAYLOAD_TEMP_OFFSET);

    /* Bytes 8-9: conteo de eventos en BIG-ENDIAN */
    out[8] = (uint8_t)((p->conteo_eventos >> 8) & 0xFF);
    out[9] = (uint8_t)(p->conteo_eventos & 0xFF);

    /* Bytes 10-11: reservado, siempre 0 */
    out[10] = 0x00;
    out[11] = 0x00;

    return PAYLOAD_OK;
}

int payload_decode(const uint8_t in[PAYLOAD_LEN], payload_t *p)
{
    if (in == 0 || p == 0) {
        return PAYLOAD_ERR_NULL;
    }

    if (in[0] != PAYLOAD_TIPO_ALARMA && in[0] != PAYLOAD_TIPO_HEARTBEAT) {
        return PAYLOAD_ERR_TIPO;
    }

    p->tipo          = in[0];
    p->fuente        = in[1];
    p->magnitud_mg   = (uint16_t)(((uint16_t)in[2] << 8) | (uint16_t)in[3]);
    p->eje           = in[4];
    p->magnetico     = in[5];
    p->bateria_pct   = in[6];
    p->temp_c        = (int8_t)((int)in[7] - PAYLOAD_TEMP_OFFSET);
    p->conteo_eventos = (uint16_t)(((uint16_t)in[8] << 8) | (uint16_t)in[9]);
    /* bytes 10-11 reservados: se ignoran al decodificar */

    return PAYLOAD_OK;
}
