//
// Created by Human Race on 26/05/2025.
//

#include "button_driver.h"
#include <mine_header.h>
#include "driver/gpio.h"
#include "button_defs.h"

static const char *TAG = "BUTTON_DRIVER";

// Queue untuk mengirim event tombol ke aplikasi utama
static QueueHandle_t s_button_event_queue = NULL;

// Array untuk menyimpan state setiap tombol
static button_t *s_buttons = NULL;
static uint8_t s_num_buttons = 0;

static button_event_type_t process_button_state(button_t *btn, bool current_gpio_state);
static void button_polling_task(void *arg);

bool button_driver_init(uint8_t num_buttons, const uint8_t button_gpios[], uint32_t event_queue_size) {
  if (num_buttons == 0 || button_gpios == NULL) {
    ESP_LOGE(TAG, "Invalid arguments: num_buttons or button_gpios is invalid.");
    return false;
  }

  s_num_buttons = num_buttons;
  s_buttons = (button_t *)calloc(s_num_buttons, sizeof(button_t)); // Alokasi memori untuk setiap tombol
  if (s_buttons == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for buttons.");
    return false;
  }

  s_button_event_queue = xQueueCreate(event_queue_size, sizeof(button_event_t)); // Ukuran item sekarang button_event_t
  if (s_button_event_queue == NULL) {
    ESP_LOGE(TAG, "Failed to create button event queue.");
    free(s_buttons);
    return false;
  }

  for (uint8_t i = 0; i < s_num_buttons; i++) {
    s_buttons[i].gpio_num = button_gpios[i];
    s_buttons[i].current_state = BTN_STATE_IDLE;
    s_buttons[i].last_press_tick = 0;
    s_buttons[i].last_release_tick = 0;
    s_buttons[i].press_count = 0;
    s_buttons[i].is_long_press_active = false;
    s_buttons[i].is_double_long_press_active = false;

    gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << s_buttons[i].gpio_num),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
    gpio_config(&io_conf);
  }

  if (xTaskCreate(button_polling_task, "ButtonPollingTask", 4096, NULL, 10, NULL) != pdPASS) {
    ESP_LOGE(TAG, "Failed to create button polling task.");
    vQueueDelete(s_button_event_queue);
    free(s_buttons);
    return false;
  }

  ESP_LOGI(TAG, "Button driver initialized for %d buttons.", s_num_buttons);
  return true;
}

bool button_driver_get_event(button_event_t *event, TickType_t timeout_ms) {
  if (s_button_event_queue == NULL) {
    ESP_LOGE(TAG, "Button event queue not initialized!");
    return false;
  }
  return xQueueReceive(s_button_event_queue, event, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

// --- Static Function ---
// Memproses event tombol berdasarkan state internal
static button_event_type_t process_button_state(button_t *btn, bool current_gpio_state) {
    button_event_type_t event = BUTTON_EVENT_NONE;
    TickType_t current_tick = xTaskGetTickCount();

    switch (btn->current_state) {
        case BTN_STATE_IDLE:
            if (!current_gpio_state) { // Tombol ditekan (GPIO LOW)
                btn->last_press_tick = current_tick;
                btn->current_state = BTN_STATE_PRESSED;
                event = BUTTON_EVENT_PRESSED;
                ESP_LOGD(TAG, "Button %d: Pressed", btn->gpio_num);
            }
            break;

        case BTN_STATE_PRESSED:
            if (current_gpio_state) { // Tombol dilepas (GPIO HIGH)
                btn->last_release_tick = current_tick;
                event = BUTTON_EVENT_RELEASED;
                ESP_LOGD(TAG, "Button %d: Released", btn->gpio_num);

                TickType_t press_duration = btn->last_release_tick - btn->last_press_tick;

                if (btn->is_long_press_active) {
                    btn->is_long_press_active = false;
                    // Long press sudah dipicu, jadi ini bukan click biasa
                    btn->current_state = BTN_STATE_IDLE; // Kembali ke idle setelah long press dilepas
                } else if (press_duration < pdMS_TO_TICKS(BUTTON_LONG_PRESS_TIME_MS)) {
                    // Ini adalah click singkat, masuk ke state menunggu double click
                    btn->current_state = BTN_STATE_WAITING_FOR_SECOND_PRESS;
                    btn->press_count = 1; // Hitung tekan pertama
                    event = BUTTON_EVENT_CLICK;
                    ESP_LOGD(TAG, "Button %d: Click", btn->gpio_num);
                } else {
                    btn->current_state = BTN_STATE_IDLE;
                }
            } else { // Tombol masih ditekan
                if (!btn->is_long_press_active && (current_tick - btn->last_press_tick >= pdMS_TO_TICKS(BUTTON_LONG_PRESS_TIME_MS))) {
                    event = BUTTON_EVENT_LONG_PRESS;
                    btn->is_long_press_active = true;
                    ESP_LOGI(TAG, "Button %d: Long Press", btn->gpio_num);
                }
            }
            break;

        case BTN_STATE_LONG_PRESS_DETECTED:
            if (current_gpio_state) { // Tombol dilepas setelah long press
                btn->is_long_press_active = false;
                btn->current_state = BTN_STATE_IDLE;
                event = BUTTON_EVENT_RELEASED;
                ESP_LOGD(TAG, "Button %d: Released after Long Press", btn->gpio_num);
            }
            break;

        case BTN_STATE_WAITING_FOR_SECOND_PRESS:
            if (!current_gpio_state) { // Tekan kedua terdeteksi
                TickType_t time_since_last_release = current_tick - btn->last_release_tick;
                if (time_since_last_release <= pdMS_TO_TICKS(BUTTON_DOUBLE_CLICK_TIMEOUT_MS)) {
                    // Tekan kedua dalam batas waktu double click
                    btn->last_press_tick = current_tick; // Reset waktu tekan untuk double long press check
                    btn->current_state = BTN_STATE_DOUBLE_CLICK_DETECTED;
                    event = BUTTON_EVENT_PRESSED; // Ini adalah event press untuk deteksi double long press
                    ESP_LOGD(TAG, "Button %d: Second Press detected", btn->gpio_num);
                } else {
                    // Terlalu lama untuk double click, kembali ke idle
                    btn->current_state = BTN_STATE_IDLE;
                    event = BUTTON_EVENT_PRESSED; // Ini adalah tekan tunggal baru
                    ESP_LOGD(TAG, "Button %d: Double click timeout, new single press", btn->gpio_num);
                }
            } else { // Tombol belum ditekan, dan timeout double click
                TickType_t time_since_first_release = current_tick - btn->last_release_tick;
                if (time_since_first_release > pdMS_TO_TICKS(BUTTON_DOUBLE_CLICK_TIMEOUT_MS)) {
                    btn->current_state = BTN_STATE_IDLE;
                    ESP_LOGD(TAG, "Button %d: Double click timeout, no second press.", btn->gpio_num);
                }
            }
            break;

        case BTN_STATE_DOUBLE_CLICK_DETECTED:
            if (current_gpio_state) { // Tombol kedua dilepas
                btn->last_release_tick = current_tick;
                event = BUTTON_EVENT_RELEASED;

                TickType_t press_duration_second_press = btn->last_release_tick - btn->last_press_tick;

                if (btn->is_double_long_press_active) {
                    btn->is_double_long_press_active = false;
                    btn->current_state = BTN_STATE_IDLE;
                    ESP_LOGD(TAG, "Button %d: Released after Double Long Press", btn->gpio_num);
                } else if (press_duration_second_press >= pdMS_TO_TICKS(BUTTON_DOUBLE_LONG_PRESS_TIME_MS)) {
                    btn->current_state = BTN_STATE_DOUBLE_LONG_PRESS_DETECTED;
                    btn->is_double_long_press_active = true;
                    event = BUTTON_EVENT_DOUBLE_LONG_PRESS;
                    ESP_LOGI(TAG, "Button %d: Double Long Press", btn->gpio_num);
                } else {
                    btn->current_state = BTN_STATE_IDLE;
                    event = BUTTON_EVENT_DOUBLE_CLICK;
                    ESP_LOGI(TAG, "Button %d: Double Click", btn->gpio_num);
                }
            } else { // Tombol kedua masih ditekan
                 if (!btn->is_double_long_press_active && (current_tick - btn->last_press_tick >= pdMS_TO_TICKS(BUTTON_DOUBLE_LONG_PRESS_TIME_MS))) {
                    event = BUTTON_EVENT_DOUBLE_LONG_PRESS;
                    btn->is_double_long_press_active = true;
                    ESP_LOGI(TAG, "Button %d: Double Long Press detected (still pressing)", btn->gpio_num);
                }
            }
            break;

        case BTN_STATE_DOUBLE_LONG_PRESS_DETECTED:
            if (current_gpio_state) { // Tombol dilepas
                btn->is_double_long_press_active = false;
                btn->current_state = BTN_STATE_IDLE;
                event = BUTTON_EVENT_RELEASED;
                ESP_LOGD(TAG, "Button %d: Released after Double Long Press", btn->gpio_num);
            }
            break;
    }

    return event;
}

// Task untuk polling status GPIO tombol
static void button_polling_task(void *arg) {
    TickType_t last_read_tick = xTaskGetTickCount();

    while (1) {
        for (uint8_t i = 0; i < s_num_buttons; i++) {
            bool current_gpio_state = gpio_get_level(s_buttons[i].gpio_num);

            // Periksa state tombol dan hasilkan event
            button_event_type_t event_type_raw = process_button_state(&s_buttons[i], current_gpio_state);

            // Jika ada event yang dihasilkan, kirim ke queue
            if (event_type_raw != BUTTON_EVENT_NONE) {
                button_event_t button_event_data = { // Buat objek event yang lengkap
                    .gpio_num = s_buttons[i].gpio_num,
                    .event_type = event_type_raw
                };
                if (xQueueSend(s_button_event_queue, &button_event_data, 0) != pdPASS) {
                    ESP_LOGW(TAG, "Button event queue full, event dropped for GPIO %d.", button_event_data.gpio_num);
                }
            }
        }
        vTaskDelayUntil(&last_read_tick, pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
    }
}