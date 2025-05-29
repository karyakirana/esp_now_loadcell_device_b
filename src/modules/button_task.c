//
// Created by Human Race on 27/05/2025.
//

#include "button_task.h"

#include "button_defs.h"
#include "esp_timer.h" // Untuk esp_timer_get_time()

static const char* TAG = "BUTTON_TASK";

static QueueHandle_t button_event_queue;

static button_info_t buttons[] = {
  { .gpio_num = BUTTON_A_GPIO, .current_state = true, .prev_state = true, .last_press_time = 0, .last_release_time = 0, .last_single_click_time = 0, .long_press_triggered = false, .is_pressed = false },
  { .gpio_num = BUTTON_B_GPIO, .current_state = true, .prev_state = true, .last_press_time = 0, .last_release_time = 0, .last_single_click_time = 0, .long_press_triggered = false, .is_pressed = false },
  { .gpio_num = BUTTON_C_GPIO, .current_state = true, .prev_state = true, .last_press_time = 0, .last_release_time = 0, .last_single_click_time = 0, .long_press_triggered = false, .is_pressed = false },
  { .gpio_num = BUTTON_D_GPIO, .current_state = true, .prev_state = true, .last_press_time = 0, .last_release_time = 0, .last_single_click_time = 0, .long_press_triggered = false, .is_pressed = false },
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))

static button_event_type_t button_handler_read_event(void);
static void send_button_event(button_event_type_t event);

esp_err_t button_task_init(void) {
  esp_err_t ret;
  gpio_config_t io_conf;
  // non-intrerrupt mode
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  // enable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  // konfigurasi setiap tombol
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    io_conf.pin_bit_mask = (1ULL << buttons[i].gpio_num);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Error configuring button %d", buttons[i].gpio_num);
      return ret;
    }
  }
  ESP_LOGI(TAG, "Buttons initialized");
  return ESP_OK;
}

bool button_task_send_to_main_queue(QueueHandle_t button_to_main_queue) {
    if (button_to_main_queue == NULL) return false;
    button_event_queue = button_to_main_queue;
    return true;
}

void button_task_update(void) {
  while (1) {
    button_event_type_t button_event = button_handler_read_event();

    switch (button_event) {
            case BUTTON_EVENT_A_SINGLE_CLICK:
                ESP_LOGI(TAG, "Button A: Single Click!");
                break;
            case BUTTON_EVENT_A_DOUBLE_CLICK:
                ESP_LOGI(TAG, "Button A: Double Click!");
                break;
            case BUTTON_EVENT_A_LONG_PRESS_START:
                ESP_LOGI(TAG, "Button A: Long Press Started!");
                break;
            case BUTTON_EVENT_A_LONG_PRESS_UP:
                ESP_LOGI(TAG, "Button A: Long Press Released!");
                break;

            case BUTTON_EVENT_B_SINGLE_CLICK:
                ESP_LOGI(TAG, "Button B: Single Click!");
                break;
            case BUTTON_EVENT_B_DOUBLE_CLICK:
                ESP_LOGI(TAG, "Button B: Double Click!");
                break;
            case BUTTON_EVENT_B_LONG_PRESS_START:
                ESP_LOGI(TAG, "Button B: Long Press Started!");
                break;
            case BUTTON_EVENT_B_LONG_PRESS_UP:
                ESP_LOGI(TAG, "Button B: Long Press Released!");
                break;

            case BUTTON_EVENT_C_SINGLE_CLICK:
                ESP_LOGI(TAG, "Button C: Single Click!");
                break;
            case BUTTON_EVENT_C_DOUBLE_CLICK:
                ESP_LOGI(TAG, "Button C: Double Click!");
                break;
            case BUTTON_EVENT_C_LONG_PRESS_START:
                ESP_LOGI(TAG, "Button C: Long Press Started!");
                break;
            case BUTTON_EVENT_C_LONG_PRESS_UP:
                ESP_LOGI(TAG, "Button C: Long Press Released!");
                break;

            case BUTTON_EVENT_D_SINGLE_CLICK:
                ESP_LOGI(TAG, "Button D: Single Click!");
                break;
            case BUTTON_EVENT_D_DOUBLE_CLICK:
                ESP_LOGI(TAG, "Button D: Double Click!");
                break;
            case BUTTON_EVENT_D_LONG_PRESS_START:
                ESP_LOGI(TAG, "Button D: Long Press Started!");
                break;
            case BUTTON_EVENT_D_LONG_PRESS_UP:
                ESP_LOGI(TAG, "Button D: Long Press Released!");
                break;

            case BUTTON_EVENT_AB_LONG_PRESS:
                ESP_LOGI(TAG, "Combination: A and B Long Press!");
                break;

            case BUTTON_NONE:
                // Tidak ada event, lakukan sesuatu yang lain atau biarkan saja
                break;
            default:
                break;
        }

    send_button_event(button_event);

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static button_event_type_t button_handler_read_event(void) {
  button_event_type_t event = BUTTON_NONE;
    uint64_t current_time_us = esp_timer_get_time();

    // Cek event untuk setiap tombol
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].current_state = gpio_get_level(buttons[i].gpio_num);

        // Jika tombol sedang ditekan (active low)
        if (buttons[i].current_state == 0) {
            if (!buttons[i].is_pressed) { // Baru mulai ditekan
                buttons[i].is_pressed = true;
                buttons[i].last_press_time = current_time_us;
                buttons[i].long_press_triggered = false;
            } else { // Sedang ditekan
                if (!buttons[i].long_press_triggered && (current_time_us - buttons[i].last_press_time) / 1000 >= LONG_PRESS_TIME_MS) {
                    buttons[i].long_press_triggered = true;
                    switch (buttons[i].gpio_num) {
                        case BUTTON_A_GPIO: event = BUTTON_EVENT_A_LONG_PRESS_START; break;
                        case BUTTON_B_GPIO: event = BUTTON_EVENT_B_LONG_PRESS_START; break;
                        case BUTTON_C_GPIO: event = BUTTON_EVENT_C_LONG_PRESS_START; break;
                        case BUTTON_D_GPIO: event = BUTTON_EVENT_D_LONG_PRESS_START; break;
                        default: break;
                    }
                    if (event != BUTTON_NONE) return event; // Prioritaskan long press start
                }
            }
        } else { // Jika tombol dilepas
            if (buttons[i].is_pressed) { // Baru saja dilepas
                buttons[i].is_pressed = false;
                buttons[i].last_release_time = current_time_us;

                if (buttons[i].long_press_triggered) {
                    switch (buttons[i].gpio_num) {
                        case BUTTON_A_GPIO: event = BUTTON_EVENT_A_LONG_PRESS_UP; break;
                        case BUTTON_B_GPIO: event = BUTTON_EVENT_B_LONG_PRESS_UP; break;
                        case BUTTON_C_GPIO: event = BUTTON_EVENT_C_LONG_PRESS_UP; break;
                        case BUTTON_D_GPIO: event = BUTTON_EVENT_D_LONG_PRESS_UP; break;
                        default: break;
                    }
                    if (event != BUTTON_NONE) return event; // Prioritaskan long press up
                } else {
                    // Cek double click
                    if ((current_time_us - buttons[i].last_single_click_time) / 1000 < DOUBLE_CLICK_TIME_MS) {
                        switch (buttons[i].gpio_num) {
                            case BUTTON_A_GPIO: event = BUTTON_EVENT_A_DOUBLE_CLICK; break;
                            case BUTTON_B_GPIO: event = BUTTON_EVENT_B_DOUBLE_CLICK; break;
                            case BUTTON_C_GPIO: event = BUTTON_EVENT_C_DOUBLE_CLICK; break;
                            case BUTTON_D_GPIO: event = BUTTON_EVENT_D_DOUBLE_CLICK; break;
                            default: break;
                        }
                        buttons[i].last_single_click_time = 0; // Reset untuk mencegah triple click yang tidak diinginkan
                        if (event != BUTTON_NONE) return event; // Prioritaskan double click
                    } else {
                        // Single click potensial, simpan waktu
                        buttons[i].last_single_click_time = current_time_us;
                    }
                }
            } else { // Tombol tidak ditekan dan stabil
                // Jika ada single click potensial yang belum diproses (waktunya habis)
                if (buttons[i].last_single_click_time != 0 && (current_time_us - buttons[i].last_single_click_time) / 1000 >= DOUBLE_CLICK_TIME_MS) {
                    switch (buttons[i].gpio_num) {
                        case BUTTON_A_GPIO: event = BUTTON_EVENT_A_SINGLE_CLICK; break;
                        case BUTTON_B_GPIO: event = BUTTON_EVENT_B_SINGLE_CLICK; break;
                        case BUTTON_C_GPIO: event = BUTTON_EVENT_C_SINGLE_CLICK; break;
                        case BUTTON_D_GPIO: event = BUTTON_EVENT_D_SINGLE_CLICK; break;
                        default: break;
                    }
                    buttons[i].last_single_click_time = 0; // Reset
                    if (event != BUTTON_NONE) return event; // Prioritaskan single click
                }
            }
        }
    }

    // Cek event kombinasi (BUTTON_EVENT_AB_LONG_PRESS)
    // Asumsi kedua tombol harus ditekan secara bersamaan dan mencapai durasi long press
    if (buttons[0].is_pressed && buttons[1].is_pressed) { // Tombol A dan B sedang ditekan
        uint64_t press_duration_A = (current_time_us - buttons[0].last_press_time) / 1000;
        uint64_t press_duration_B = (current_time_us - buttons[1].last_press_time) / 1000;

        // Cek jika kedua tombol sudah melewati durasi long press
        // Dan pastikan tidak ada long press individual yang terdeteksi untuk A atau B
        if (press_duration_A >= LONG_PRESS_TIME_MS && press_duration_B >= LONG_PRESS_TIME_MS) {
            // Untuk memastikan hanya terdeteksi sekali saat kombinasi long press dimulai
            if (!buttons[0].long_press_triggered && !buttons[1].long_press_triggered) {
                event = BUTTON_EVENT_AB_LONG_PRESS;
                // Set long_press_triggered untuk kedua tombol agar tidak re-trigger
                buttons[0].long_press_triggered = true;
                buttons[1].long_press_triggered = true;
                return event;
            }
        }
    } else {
        // Reset long_press_triggered jika kombinasi dilepas
        if (buttons[0].long_press_triggered && buttons[1].long_press_triggered) {
            buttons[0].long_press_triggered = false;
            buttons[1].long_press_triggered = false;
        }
    }

    return event;
}

static void send_button_event(button_event_type_t event) {
  if (xQueueSend(button_event_queue, &event, pdMS_TO_TICKS(200)) != pdPASS) {
    ESP_LOGW(TAG, "Failed to send button event");
  } else {
    ESP_LOGE(TAG, "Button event: %d", event);
  }
}