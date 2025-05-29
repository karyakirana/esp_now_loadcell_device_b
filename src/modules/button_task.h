//
// Created by Human Race on 27/05/2025.
//

#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <mine_header.h>
#include "driver/gpio.h"

// Definisi GPIO untuk setiap tombol
#define BUTTON_A_GPIO GPIO_NUM_13 // Sebelumnya GPIO_NUM_0
#define BUTTON_B_GPIO GPIO_NUM_12 // Sebelumnya GPIO_NUM_1
#define BUTTON_C_GPIO GPIO_NUM_14 // Sebelumnya GPIO_NUM_2
#define BUTTON_D_GPIO GPIO_NUM_27 // Sebelumnya GPIO_NUM_3

// Waktu debounce dalam milidetik
#define DEBOUNCE_TIME_MS 100

// Waktu double click dalam milidetik
#define DOUBLE_CLICK_TIME_MS 300

// Waktu long press dalam milidetik
#define LONG_PRESS_TIME_MS 1000

typedef enum  {
  BUTTON_NONE = 0,
  // Event untuk Tombol A (GPIO_NUM_0)
  BUTTON_EVENT_A_SINGLE_CLICK,
  BUTTON_EVENT_A_DOUBLE_CLICK,
  BUTTON_EVENT_A_LONG_PRESS_START,
  BUTTON_EVENT_A_LONG_PRESS_UP,

  // Event untuk Tombol B (GPIO_NUM_1)
  BUTTON_EVENT_B_SINGLE_CLICK,
  BUTTON_EVENT_B_DOUBLE_CLICK,
  BUTTON_EVENT_B_LONG_PRESS_START,
  BUTTON_EVENT_B_LONG_PRESS_UP,

  // Event untuk Tombol C (GPIO_NUM_2)
  BUTTON_EVENT_C_SINGLE_CLICK,
  BUTTON_EVENT_C_DOUBLE_CLICK,
  BUTTON_EVENT_C_LONG_PRESS_START,
  BUTTON_EVENT_C_LONG_PRESS_UP,

  // Event untuk Tombol D (GPIO_NUM_3)
  BUTTON_EVENT_D_SINGLE_CLICK,
  BUTTON_EVENT_D_DOUBLE_CLICK,
  BUTTON_EVENT_D_LONG_PRESS_START,
  BUTTON_EVENT_D_LONG_PRESS_UP,

  // Event Kombinasi
  BUTTON_EVENT_AB_LONG_PRESS,     // Tombol A dan B ditekan lama bersamaan
} button_event_type_t;

// Struktur untuk menyimpan status setiap tombol
typedef struct {
  gpio_num_t gpio_num;
  bool current_state;
  bool prev_state;
  uint64_t last_press_time;
  uint64_t last_release_time;
  uint64_t last_single_click_time;
  bool long_press_triggered;
  bool is_pressed; // True jika tombol sedang ditekan
} button_info_t;

esp_err_t  button_task_init(void);

bool button_task_send_to_main_queue(QueueHandle_t button_to_main_queue);

void button_task_update(void); // button loop

#endif //BUTTON_TASK_H
