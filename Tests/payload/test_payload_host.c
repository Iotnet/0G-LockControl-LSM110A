/**
  ******************************************************************************
  * @file    test_payload_host.c
  * @brief   Test host-side del codec del payload (sin HAL). Codifica los 5
  *          vectores compartidos y los compara contra el hex esperado de
  *          Tests/payload/test_vectors.md. Tambien hace round-trip decode.
  * @author  0G IoT Solutions
  *
  * @note    Compilar y ejecutar desde Tests/payload/ con gcc:
  *
  *      gcc -std=c99 -Wall -Wextra \
  *          -I../../Firmware/app/payload \
  *          ../../Firmware/app/payload/payload_codec.c \
  *          test_payload_host.c -o test_payload_host && ./test_payload_host
  *
  *      (Windows / MinGW: usar el mismo comando; el binario sera
  *       test_payload_host.exe)
  *
  * @retval 0 si los 5 vectores pasan, 1 si alguno falla.
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include "payload_codec.h"

/* Convierte 12 bytes a string hex en MAYUSCULAS (24 chars + '\0'). */
static void to_hex(const uint8_t *buf, char *out)
{
    static const char H[] = "0123456789ABCDEF";
    for (int i = 0; i < PAYLOAD_LEN; i++) {
        out[i * 2]     = H[(buf[i] >> 4) & 0xF];
        out[i * 2 + 1] = H[buf[i] & 0xF];
    }
    out[PAYLOAD_LEN * 2] = '\0';
}

typedef struct {
    const char *nombre;
    payload_t   in;
    const char *hex_esperado;
} caso_t;

int main(void)
{
    caso_t casos[] = {
        {
            "V1 alarma accel",
            { PAYLOAD_TIPO_ALARMA, PAYLOAD_FUENTE_ACCEL, 200, PAYLOAD_EJE_Z,
              PAYLOAD_MAG_CERRADO, 90, 25, 15 },
            "010100C803005A41000F0000"
        },
        {
            "V2 alarma magnetico",
            { PAYLOAD_TIPO_ALARMA, PAYLOAD_FUENTE_MAGNETICO, 0, PAYLOAD_EJE_NINGUNO,
              PAYLOAD_MAG_ABIERTO, 88, 22, 16 },
            "010200000001583E00100000"
        },
        {
            "V3 alarma dual",
            { PAYLOAD_TIPO_ALARMA, PAYLOAD_FUENTE_ACCEL | PAYLOAD_FUENTE_MAGNETICO,
              350, PAYLOAD_EJE_X, PAYLOAD_MAG_ABIERTO, 85, 30, 17 },
            "0103015E0101554600110000"
        },
        {
            "V4 heartbeat",
            { PAYLOAD_TIPO_HEARTBEAT, PAYLOAD_FUENTE_NINGUNA, 0, PAYLOAD_EJE_NINGUNO,
              PAYLOAD_MAG_CERRADO, 75, 4, 0 },
            "0200000000004B2C00000000"
        },
        {
            "V5 temp negativa",
            { PAYLOAD_TIPO_HEARTBEAT, PAYLOAD_FUENTE_NINGUNA, 0, PAYLOAD_EJE_NINGUNO,
              PAYLOAD_MAG_CERRADO, 60, -10, 0 },
            "0200000000003C1E00000000"
        },
    };

    const int n = (int)(sizeof(casos) / sizeof(casos[0]));
    int fallos = 0;

    for (int i = 0; i < n; i++) {
        uint8_t buf[PAYLOAD_LEN];
        char hex[PAYLOAD_LEN * 2 + 1];

        /* 1) encode + comparacion de hex */
        if (payload_encode(&casos[i].in, buf) != PAYLOAD_OK) {
            printf("[FAIL] %-22s encode devolvio error\n", casos[i].nombre);
            fallos++;
            continue;
        }
        to_hex(buf, hex);

        int ok_hex = (strcmp(hex, casos[i].hex_esperado) == 0);

        /* 2) round-trip decode: los campos deben volver iguales */
        payload_t back;
        int ok_rt = (payload_decode(buf, &back) == PAYLOAD_OK) &&
                    back.tipo == casos[i].in.tipo &&
                    back.fuente == casos[i].in.fuente &&
                    back.magnitud_mg == casos[i].in.magnitud_mg &&
                    back.eje == casos[i].in.eje &&
                    back.magnetico == casos[i].in.magnetico &&
                    back.bateria_pct == casos[i].in.bateria_pct &&
                    back.temp_c == casos[i].in.temp_c &&
                    back.conteo_eventos == casos[i].in.conteo_eventos;

        if (ok_hex && ok_rt) {
            printf("[PASS] %-22s %s\n", casos[i].nombre, hex);
        } else {
            fallos++;
            printf("[FAIL] %-22s\n", casos[i].nombre);
            if (!ok_hex) {
                printf("        esperado: %s\n", casos[i].hex_esperado);
                printf("        obtenido: %s\n", hex);
            }
            if (!ok_rt) {
                printf("        round-trip decode no coincide con el origen\n");
            }
        }
    }

    printf("\n%d/%d vectores OK\n", n - fallos, n);
    return fallos == 0 ? 0 : 1;
}
