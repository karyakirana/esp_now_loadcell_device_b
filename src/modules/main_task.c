//
// Created by Human Race on 26/05/2025.
//

#include "main_task.h"

#include <hal/gpio_types.h>

#include "button.h"
#include "button_task.h"

static const char *TAG = "MAIN_TASK";

main_state_t current_state = NORMAL_MODE;
calibration_state_t calibration_state = CAL_UNKNOWN;

QueueHandle_t main_from_button_handler;
QueueHandle_t main_to_oled_handler;
QueueHandle_t main_from_comm_handler;
QueueHandle_t main_to_com_handler;

weight_data_t weight_data;
led_data_t led_data;
button_event_type_t button_event;

char buffer_1[16];
char buffer_2[16];

// forward declaration
static void rcv_queue_from_button_handler(void);
static void rcv_queue_from_comm_handler(void);
static void send_queue_to_led_handler(void);

// handler queue by state
static void normal_mode_handler(void);
static void sleep_mode_handler(void);
static void deep_sleep_handler(void);
static void wake_up_handler(void);

static void cal_init_handler(void);
static void cal_waiting_handler(void);
static void cal_input_handler(void);
static void cal_confirmation_handler(void);

// menerima pesan dan meneruskan ke handler (main state)
static void main_state_queue_dispatcher(void);
static void main_sub_state_queue_dispatcher(void);

// mengelola pesan dan mengirimkan ke queue
static void lcd_queue_handler(void);
static void comm_queue_handler(void);

void main_task_init(void) {
}

bool main_task_rcv_button(QueueHandle_t rcv_btn_queue) {
  if (rcv_btn_queue == NULL) return false;
  main_from_button_handler = rcv_btn_queue;
  return true;
}

bool main_task_rcv_comm(QueueHandle_t rcv_comm_queue) {
  if (rcv_comm_queue == NULL) return false;
  main_from_comm_handler = rcv_comm_queue;
  return true;
}

bool main_task_send_comm(QueueHandle_t send_comm_queue) {
  if (send_comm_queue == NULL) return false;
  main_to_com_handler = send_comm_queue;
  return true;
}

bool main_task_send_oled(QueueHandle_t send_oled_queue) {
  if (send_oled_queue == NULL) return false;
  main_to_oled_handler = send_oled_queue;
  return true;
}

void main_task_update(void) {
  while (1) {

    // todo: receive queue and dispatch
    main_state_queue_dispatcher();

    // TODO: process data oled by state

    // TODO: process data comm by state

    send_queue_to_led_handler();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// --- static function ---
static void rcv_queue_from_button_handler(void) {
  if (xQueueReceive(main_from_button_handler, &button_event, pdMS_TO_TICKS(100)) == pdPASS) {
    ESP_LOGI(TAG, "Got button event");
    if (button_event == BUTTON_EVENT_AB_LONG_PRESS) {
      current_state = (NORMAL_MODE) ? CALIBRATION_MODE : NORMAL_MODE;
    }
  } else {
    ESP_LOGW(TAG, "Got unexpected button event");
    button_event = BUTTON_NONE;
  }
}

static void rcv_queue_from_comm_handler(void) {

  if (xQueueReceive(main_from_comm_handler, &weight_data, pdMS_TO_TICKS(100)) == pdPASS) {
    //
    ESP_LOGI(TAG, "Units: %.2f", weight_data.units ? weight_data.units : 0.0f);
    ESP_LOGI(TAG, "Raw: %ld", weight_data.raw_weight ? weight_data.raw_weight : 0);
  } else {
    ESP_LOGW(TAG, "main_task_rcv_comm_handler timeout");
  }
}

static void send_queue_to_led_handler(void) {

  if (!weight_data.units) return;
  if (!weight_data.raw_weight) return;

  sprintf(buffer_1, "%ld", weight_data.raw_weight);
  // buffer_1[strlen(buffer_1)-1] = '\0';
  sprintf(buffer_2, "%.2f", weight_data.units);
  // buffer_2[strlen(buffer_2)-1] = '\0';

  led_data.line_1 = buffer_1;
  led_data.line_2 = buffer_2;
  ESP_LOGI(TAG, "Buffer 1: %s", led_data.line_1);

  if (xQueueSend(main_to_oled_handler, &led_data, pdMS_TO_TICKS(100)) != pdPASS) {
    ESP_LOGW(TAG, "main_task_send_oled: timeout");
  }
}

static void main_state_queue_dispatcher(void) {
  if (current_state == CALIBRATION_MODE) {
    main_sub_state_queue_dispatcher();
    return;
  }

  // isi data dari button
  rcv_queue_from_button_handler();

  // isi data dari queue comm
  rcv_queue_from_comm_handler();

  switch (current_state) {
    case SLEEP_MODE:
      sleep_mode_handler();
      break;
    case DEEPSLEEP_MODE:
      deep_sleep_handler();
      break;
    case WAKE_UP_MODE:
      wake_up_handler();
      break;
    case NORMAL_MODE:
    default:
      normal_mode_handler();
      break;
  }
}

static void main_sub_state_queue_dispatcher(void) {

  // isi data dari button
  rcv_queue_from_button_handler();

  // isi data dari queue comm
  rcv_queue_from_comm_handler();

  switch (calibration_state) {
    case CAL_INIT:
      cal_init_handler();
      break;
    case CAL_WAITING:
      cal_waiting_handler();
      break;
    case CAL_INPUT:
      cal_input_handler();
      break;
    case CAL_CONFIRMATION:
      cal_confirmation_handler();
      break;
    default:
      break;
  }
}

static void normal_mode_handler(void) {}
static void sleep_mode_handler(void) {}
static void deep_sleep_handler(void) {}
static void wake_up_handler(void) {}

static void cal_init_handler(void) {}
static void cal_waiting_handler(void) {}
static void cal_input_handler(void) {}
static void cal_confirmation_handler(void) {}