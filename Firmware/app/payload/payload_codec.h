/**
  ******************************************************************************
  * @file    payload_codec.h
  * @brief   Codec del payload Sigfox de 12 bytes para 0G LockControl
  * @author  0G IoT Solutions
  *
  * @note    C puro, SIN dependencias de HAL. Compilable en host (gcc/clang) para
  *          poder generar y validar vectores de prueba fuera del firmware.
  *
  * @note    Contrato del payload (12 bytes, big-endian en los campos uint16):
  *
  *          Byte  0      tipo        (0x01 alarma, 0x02 heartbeat)
  *          Byte  1      fuente      bitfield (bit0 accel, bit1 magnetico)
  *          Bytes 2-3    magnitud_mg uint16 BIG-ENDIAN (mg de aceleracion)
  *          Byte  4      eje         (0 ninguno, 1 X, 2 Y, 3 Z)
  *          Byte  5      magnetico   (0 cerrado, 1 abierto)
  *          Byte  6      bateria_pct (0..100)
  *          Byte  7      temp        (temp_c + 40, offset para soportar negativos)
  *          Bytes 8-9    conteo      uint16 BIG-ENDIAN (conteo de eventos)
  *          Bytes 10-11  reservado   (siempre 0x0000)
  *
  ******************************************************************************
  */

#ifndef PAYLOAD_CODEC_H
#define PAYLOAD_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Longitud fija del payload Sigfox en bytes */
#define PAYLOAD_LEN              12

/* ---- Byte 0: tipo de mensaje ---- */
#define PAYLOAD_TIPO_ALARMA      0x01
#define PAYLOAD_TIPO_HEARTBEAT   0x02

/* ---- Byte 1: fuente (bitfield, se pueden combinar con OR) ---- */
#define PAYLOAD_FUENTE_NINGUNA     0x00
#define PAYLOAD_FUENTE_ACCEL       0x01   /* bit0: disparo por acelerometro  */
#define PAYLOAD_FUENTE_MAGNETICO   0x02   /* bit1: disparo por sensor magnetico */

/* ---- Byte 4: eje que disparo (acelerometro) ---- */
#define PAYLOAD_EJE_NINGUNO        0x00
#define PAYLOAD_EJE_X              0x01
#define PAYLOAD_EJE_Y              0x02
#define PAYLOAD_EJE_Z              0x03

/* ---- Byte 5: estado del sensor magnetico ---- */
#define PAYLOAD_MAG_CERRADO        0x00
#define PAYLOAD_MAG_ABIERTO        0x01

/* ---- Byte 7: offset de temperatura (temp_c + offset) ---- */
#define PAYLOAD_TEMP_OFFSET        40

/* Codigos de retorno */
#define PAYLOAD_OK                 0
#define PAYLOAD_ERR_NULL          -1   /* puntero nulo en parametros        */
#define PAYLOAD_ERR_TIPO          -2   /* tipo desconocido al decodificar   */

/**
 * @brief Representacion en memoria del payload (campos ya interpretados).
 *
 * @note  temp_c es la temperatura REAL en grados C (puede ser negativa). El
 *        offset +40 solo existe en la representacion serializada (byte 7).
 *        Rango representable con int8_t y offset 40: -40 .. +87 grados C.
 */
typedef struct {
    uint8_t  tipo;            /* PAYLOAD_TIPO_*                              */
    uint8_t  fuente;          /* OR de PAYLOAD_FUENTE_*                      */
    uint16_t magnitud_mg;     /* magnitud de aceleracion en mg              */
    uint8_t  eje;             /* PAYLOAD_EJE_*                              */
    uint8_t  magnetico;       /* PAYLOAD_MAG_*                              */
    uint8_t  bateria_pct;     /* nivel de bateria 0..100                    */
    int8_t   temp_c;          /* temperatura real en grados C (sin offset)  */
    uint16_t conteo_eventos;  /* contador de eventos                        */
} payload_t;

/**
 * @brief Serializa un payload_t a 12 bytes segun el contrato.
 *
 * @param[in]  p    estructura origen (no nula)
 * @param[out] out  buffer de exactamente PAYLOAD_LEN bytes
 *
 * @retval PAYLOAD_OK        en exito
 * @retval PAYLOAD_ERR_NULL  si p u out son nulos
 *
 * @note Big-endian para magnitud_mg (bytes 2-3) y conteo_eventos (bytes 8-9).
 *       Los bytes 10-11 se escriben siempre en 0x00.
 */
int payload_encode(const payload_t *p, uint8_t out[PAYLOAD_LEN]);

/**
 * @brief Deserializa 12 bytes a un payload_t. Pensado para tests en host y
 *        para la ruta de downlink futura.
 *
 * @param[in]  in  buffer de exactamente PAYLOAD_LEN bytes
 * @param[out] p   estructura destino (no nula)
 *
 * @retval PAYLOAD_OK        en exito
 * @retval PAYLOAD_ERR_NULL  si in o p son nulos
 * @retval PAYLOAD_ERR_TIPO  si in[0] no es un tipo conocido
 */
int payload_decode(const uint8_t in[PAYLOAD_LEN], payload_t *p);

#ifdef __cplusplus
}
#endif

#endif /* PAYLOAD_CODEC_H */
