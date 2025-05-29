#include <mine_header.h>
#include <button_defs.h>
#include "modules/main_task.h"
#include "modules/comm_task.h"
#include "nvs_flash.h"
#include "modules/lcd_task.h"
#include "modules/button_task.h"

static const char* TAG = "MAIN";

// static var
main_state_t main_state;
weight_data_t weight_data;
led_data_t led_data;
comm_send_data_t comm_send;

// Queue Handle
QueueHandle_t main_to_led_queue;
QueueHandle_t main_to_comm_queue;
QueueHandle_t comm_to_main_queue;
QueueHandle_t button_to_main_queue;

// declaration task func
static void main_task(void *pvParameters);
static void comm_task(void *pvParameters);
static void led_task(void *pvParameters);
static void button_task(void *pvParameters);

static void main_var_init(void);

void app_main() {
  // debug type
  esp_log_level_set("BUTTON_TASK", ESP_LOG_INFO);

  // isi variabel dengan nilai awal
  main_var_init();

  // init task

  // comm_task
  // Inisialisasi NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(comm_task_init());

  // Main Task -> led task queue
  main_to_led_queue = xQueueCreate(10, sizeof(led_data_t));
  if (main_to_led_queue == NULL) {
    ESP_LOGE(TAG, "main_to_led_queue is NULL");
  }

  main_to_comm_queue = xQueueCreate(10, sizeof(comm_send_data_t));
  if (main_to_comm_queue == NULL) {
    ESP_LOGE(TAG, "main_to_comm_queue is NULL");
  }

  comm_to_main_queue = xQueueCreate(10, sizeof(weight_data_t));
  if (comm_to_main_queue == NULL) {
    ESP_LOGE(TAG, "comm_to_main_queue is NULL");
  }

  button_to_main_queue = xQueueCreate(5, sizeof(button_event_type_t));
  if (button_to_main_queue == NULL) {
    ESP_LOGE(TAG, "button_to_main_queue is NULL");
  }

  // create task
  xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
  xTaskCreate(comm_task, "comm_task", 4096, NULL, 5, NULL);
  xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
  xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
}

static void main_task(void *pvParameters) {

  // main task init
  main_task_init();

  if (!main_task_rcv_comm(comm_to_main_queue)) {
    ESP_LOGE(TAG, "main_task_rcv_comm failed");
  }

  if (!main_task_send_comm(main_to_comm_queue)) {
    ESP_LOGE(TAG, "main_task_send_comm failed");
  }

  if (!main_task_send_oled(main_to_led_queue)) {
    ESP_LOGE(TAG, "main_task_send_oled failed");
  }

  if (!main_task_rcv_button(button_to_main_queue)) {
    ESP_LOGE(TAG, "main_task_rcv_button failed");
  }

  main_task_update();
}

static void comm_task(void *pvParameters) {
  // menerima pesan dari ESP32_A
  if (!comm_task_send_to_main_queue(comm_to_main_queue)) {
    ESP_LOGW(TAG, "comm_task_send_to_main_queue failed");
  }

  if (!comm_task_rcv_from_main_queue(main_to_comm_queue)) {
    ESP_LOGW(TAG, "comm_task_rcv_from_main_queue rcv failed");
  }

  comm_task_update();
}

static void led_task(void *pvParameters) {

  // lcd init
  lcd_task_init();

  if (!lcd_queue_handler_from_main(main_to_led_queue)) {
    ESP_LOGW(TAG, "lcd_queue_handler_from_main failed");
  }

  lcd_task_update();
}

static void button_task(void *pvParameters) {
  button_task_init();

  if (button_task_send_to_main_queue(button_to_main_queue)) {
    ESP_LOGW(TAG, "button_task_send_to_main_queue failed");
  }

  button_task_update();
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