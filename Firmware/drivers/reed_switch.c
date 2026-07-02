/**
  ******************************************************************************
  * @file    reed_switch.c
  * @brief   Implementacion del driver del sensor magnetico con debounce.
  * @author  Yahir Flores - 0G IoT Solutions
  ******************************************************************************
  */

#include "reed_switch.h"

void reed_switch_init(reed_switch_t *dev,
                      GPIO_TypeDef *port,
                      uint16_t pin,
                      uint16_t debounce_ms)
{
    if (dev == NULL) return;
    dev->port = port;
    dev->pin = pin;
    dev->debounce_ms = (debounce_ms == 0) ? REED_DEBOUNCE_MS_DEFAULT : debounce_ms;
    dev->last_event_tick = 0;
    dev->event_pending = false;
    /* Lectura inicial para sincronizar last_state */
    dev->last_state = reed_switch_read_now(dev);
}

reed_state_t reed_switch_read_now(reed_switch_t *dev)
{
    if (dev == NULL || dev->port == NULL) return REED_CLOSED;
    GPIO_PinState p = HAL_GPIO_ReadPin(dev->port, dev->pin);
    /* Pull-up: LOW = iman presente = CLOSED, HIGH = sin iman = OPEN */
    return (p == GPIO_PIN_RESET) ? REED_CLOSED : REED_OPEN;
}

void reed_switch_on_interrupt(reed_switch_t *dev)
{
    if (dev == NULL) return;

    uint32_t now = HAL_GetTick();
    uint32_t delta = now - dev->last_event_tick;  /* wraparound seguro en uint32 */

    if (delta < dev->debounce_ms) {
        /* Dentro del rango de rebote: ignorar */
        return;
    }
    dev->last_event_tick = now;
    dev->event_pending = true;
}

bool reed_switch_has_pending_event(reed_switch_t *dev)
{
    return (dev != NULL) && dev->event_pending;
}

void reed_switch_process_event(reed_switch_t *dev, reed_state_t *state)
{
    if (dev == NULL) return;
    reed_state_t s = reed_switch_read_now(dev);
    dev->last_state = s;
    dev->event_pending = false;
    if (state != NULL) *state = s;
}
