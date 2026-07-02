/**
  ******************************************************************************
  * @file    reed_switch.h
  * @brief   Driver del sensor magnetico (reed switch o DRV5032 hall) con
  *          debounce por software.
  * @author  Yahir Flores - 0G IoT Solutions
  *
  * @note    Pinout (LSM110A):
  *          - Entrada: PA1 (EXTI1)
  *          - Reed switch normalmente-abierto (NA): cerrado con iman.
  *            Pull-up interno => pin lee LOW con iman, HIGH sin iman.
  *          - DRV5032: misma convencion logica (open-drain, pull-up).
  *
  * @note    Spec sec 4.4: byte 5 del payload = "estado magnetico"
  *          0 = cerrado (iman presente, puerta cerrada)
  *          1 = abierto (sin iman, alarma)
  ******************************************************************************
  */

#ifndef REED_SWITCH_H
#define REED_SWITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32wlxx_hal.h"

#define REED_DEBOUNCE_MS_DEFAULT   50U

/**
 * @brief Estado logico del sensor magnetico.
 */
typedef enum {
    REED_CLOSED = 0,   /* iman presente => puerta cerrada */
    REED_OPEN   = 1,   /* sin iman => puerta abierta (alarma) */
} reed_state_t;

/**
 * @brief Handle del driver.
 */
typedef struct {
    GPIO_TypeDef *port;          /* p.ej. GPIOA */
    uint16_t pin;                /* p.ej. GPIO_PIN_1 */
    uint16_t debounce_ms;        /* tiempo minimo entre eventos */
    volatile uint32_t last_event_tick;
    volatile bool event_pending; /* set en ISR si pasa el debounce */
    volatile reed_state_t last_state;
} reed_switch_t;

/**
 * @brief Inicializa el driver. NO configura el pin (eso lo hace CubeMX).
 *
 * @param port        Puerto GPIO (ej. GPIOA)
 * @param pin         Mascara del pin (ej. GPIO_PIN_1)
 * @param debounce_ms Tiempo minimo entre eventos. Default 50.
 */
void reed_switch_init(reed_switch_t *dev,
                      GPIO_TypeDef *port,
                      uint16_t pin,
                      uint16_t debounce_ms);

/**
 * @brief Llamar desde HAL_GPIO_EXTI_Callback cuando GPIO_Pin == dev->pin.
 *        Aplica el debounce con HAL_GetTick(). Seguro para ISR.
 */
void reed_switch_on_interrupt(reed_switch_t *dev);

/**
 * @brief Devuelve true si hay un evento pendiente sin procesar.
 */
bool reed_switch_has_pending_event(reed_switch_t *dev);

/**
 * @brief Procesa el evento: lee el estado actual del pin y limpia el flag.
 *        Llamar desde main loop.
 *
 * @param[out] state Estado leido (CLOSED u OPEN).
 */
void reed_switch_process_event(reed_switch_t *dev, reed_state_t *state);

/**
 * @brief Lectura inmediata del pin (sin debounce). Util para el payload
 *        de heartbeat o para inicializar last_state al boot.
 */
reed_state_t reed_switch_read_now(reed_switch_t *dev);

#endif /* REED_SWITCH_H */
