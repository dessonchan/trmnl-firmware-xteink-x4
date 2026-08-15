#include <Arduino.h>
#include <config.h>
#include <pins.h>

#ifdef BOARD_XTEINK_X4
#include "x4_buttons.h"
#endif

void pins_init(void) {
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << PIN_INTERRUPT);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

#ifdef BOARD_XTEINK_X4
  // Initialize ADC pins for side buttons (GPIO1/GPIO2 resistor ladder)
  x4_buttons_init();
#endif
}
