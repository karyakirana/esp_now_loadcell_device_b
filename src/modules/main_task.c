//
// Created by Human Race on 26/05/2025.
//

#include "main_task.h"

#include <hal/gpio_types.h>

#include "drivers/button_driver.h"


#define BUTTON_GPIO_1 GPIO_NUM_13
#define BUTTON_GPIO_2 GPIO_NUM_12
#define BUTTON_GPIO_3 GPIO_NUM_14
#define BUTTON_GPIO_4 GPIO_NUM_27

static const char *TAG = "MAIN_TASK";


button_event_t received_btn_event;

main_state_t current_state = NORMAL_MODE;

static void change_main_state(button_event_t *button_event);

void main_task_init(void) {
  // init array button
  const uint8_t my_buttons[] = { BUTTON_GPIO_1, BUTTON_GPIO_2, BUTTON_GPIO_3, BUTTON_GPIO_4 };
  uint8_t num_my_buttons = sizeof(my_buttons) / sizeof(my_buttons[0]);

  // init button driver
  if (!button_driver_init(num_my_buttons, my_buttons, 10)) {
    ESP_LOGE(TAG, "Button driver init failed");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }
}

bool main_change_state(main_state_t *main_state) {
  if (*main_state == current_state) {
    return false;
  }
  *main_state = current_state;
  return true;
}

bool main_queue_rcv_comm(QueueHandle_t comm_queue, weight_data_t weight_data) {
  return true;
}

bool main_queue_rcv_btn() {
  //
  if (button_driver_get_event(&received_btn_event, portMAX_DELAY)) {
    ESP_LOGI(TAG, "Event dari GPIO %d:", received_btn_event.gpio_num);
    // dispatching
    change_main_state(&received_btn_event);
    return true;
  }
  return false;
}

bool main_queue_send_comm(QueueHandle_t main_to_comm_queue, comm_send_data_t *comm_send_data) {
  return true;
}

bool main_queue_send_led(QueueHandle_t main_to_led_queue, led_data_t *led_data) {
  return true;
}

// --- static function ---
static void change_main_state(button_event_t *button_event) {
  // button a dan b long press
}