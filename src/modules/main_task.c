//
// Created by Human Race on 26/05/2025.
//

#include "main_task.h"

#include <hal/gpio_types.h>

static const char *TAG = "MAIN_TASK";

main_state_t current_state = NORMAL_MODE;

void main_task_init(void) {
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
  return false;
}

bool main_queue_send_comm(QueueHandle_t main_to_comm_queue, comm_send_data_t *comm_send_data) {
  return true;
}

bool main_queue_send_led(QueueHandle_t main_to_led_queue, led_data_t *led_data) {
  return true;
}