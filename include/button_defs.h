//
// Created by Human Race on 26/05/2025.
//

#ifndef BUTTON_DEFS_H
#define BUTTON_DEFS_H
#include <hal/gpio_types.h>

// Existing enums dan structs
typedef enum {
  BUTTON_STATE_RELEASED = 0,
  BUTTON_STATE_PRESSED
} button_state_t;

typedef enum {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_PRESS,
  BUTTON_EVENT_RELEASE,
  BUTTON_EVENT_CLICK,
  BUTTON_EVENT_LONG_PRESS,
  // FOR COMBINED PRESS
  BUTTON_EVENT_COMBINED_PRESS,
  BUTTON_EVENT_COMBINED_LONG_PRESS,
} button_event_t;

typedef struct {
  gpio_num_t gpio_num;
  gpio_pull_mode_t pull_mode;
  uint32_t active_level;
  uint32_t debounce_time_ms;
  uint32_t long_press_time_ms;
} button_config_t;

#endif //BUTTON_DEFS_H
