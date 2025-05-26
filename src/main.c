#include <mine_header.h>
#include <button_defs.h>
#include "modules/main_task.h"

static const char* TAG = "MAIN";

// static var
main_state_t main_state;
weight_data_t weight_data;
led_data_t led_data;
comm_send_data_t comm_send;
button_event_t button_received_event;

// Queue Handle
QueueHandle_t main_to_led_queue;
QueueHandle_t main_to_comm_queue;
QueueHandle_t comm_to_main_queue;

// declaration task func
static void main_task(void *pvParameters);
static void comm_task(void *pvParameters);
static void led_task(void *pvParameters);

static void main_var_init(void);

void app_main() {
  // debug type
  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  // isi variabel dengan nilai awal
  main_var_init();

  // init task

  // Main Task -> led task queue
  main_to_led_queue = xQueueCreate(10, sizeof(char));
  if (main_to_led_queue == NULL) {
    ESP_LOGE(TAG, "main_to_led_queue is NULL");
  }

  main_to_comm_queue = xQueueCreate(10, sizeof(char));
  if (main_to_comm_queue == NULL) {
    ESP_LOGE(TAG, "main_to_comm_queue is NULL");
  }

  comm_to_main_queue = xQueueCreate(10, sizeof(char));
  if (comm_to_main_queue == NULL) {
    ESP_LOGE(TAG, "comm_to_main_queue is NULL");
  }

  // create task
  xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
  xTaskCreate(comm_task, "comm_task", 4096, NULL, 5, NULL);
  xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
}

static void main_task(void *pvParameters) {

  // main task init
  main_task_init();

  while (1) {
    // menerima pesan dari comm_task

    // mengirim pesan ke led_task

    // menerima pesan dari button_task
    if (!main_queue_rcv_btn()) {
      ESP_LOGW(TAG, "button_received_event rcv failed");
    }

    // mengitim pesan ke comm_task

    // change state
    main_change_state(&main_state);
  }
}

static void comm_task(void *pvParameters) {
  while (1) {}
}

static void led_task(void *pvParameters) {
  while (1) {}
}

static void main_var_init(void) {
  main_state = NORMAL_MODE;

  weight_data.main_state = main_state;
  weight_data.filtered_weight = 0.0f;
  weight_data.units = 0.0f;
  weight_data.raw_weight = 0;
  weight_data.is_ready = false;

  led_data.line_1 = "";
  led_data.line_2 = "";

  comm_send.command = CMD_NORMAL;
  comm_send.value = 0.0f;
}