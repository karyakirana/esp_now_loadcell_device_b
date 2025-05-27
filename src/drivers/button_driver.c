//
// Created by Human Race on 26/05/2025.
//

#include "button_driver.h"
#include <mine_header.h>
#include "driver/gpio.h"
#include "button_defs.h"
#include "freertos/timers.h" // Untuk FreeRTOS Timers

static const char *TAG = "BUTTON_DRIVER";

// --- Struktur Data Internal ---

// Maksimal tombol yang bisa didaftarkan
#define MAX_BUTTONS 5
// Maksimal kombinasi tombol yang bisa didaftarkan
#define MAX_COMBINED_BUTTONS 3

// Struktur untuk melacak status setiap tombol yang terdaftar
typedef struct {
    gpio_num_t gpio_num;
    gpio_pull_mode_t pull_mode;
    uint32_t active_level;
    uint32_t debounce_time_ms;
    uint32_t long_press_time_ms;
    uint64_t press_start_time_ms; // Waktu ketika tombol mulai ditekan
    button_state_t current_state; // Status tombol saat ini (setelah debounce)
    bool long_press_triggered;    // Flag untuk memastikan long press hanya sekali pemicuan

    // Array untuk menyimpan callback event tunggal
    // (BUTTON_EVENT_NONE, BUTTON_EVENT_PRESS, BUTTON_EVENT_RELEASE, BUTTON_EVENT_CLICK, BUTTON_EVENT_LONG_PRESS)
    button_callback_t callbacks[BUTTON_EVENT_LONG_PRESS + 1];

    TimerHandle_t debounce_timer; // Timer untuk debounce
    TimerHandle_t long_press_timer; // Timer untuk long press
} button_info_t;

// Struktur untuk melacak kombinasi tombol
typedef struct {
    gpio_num_t button1_gpio_num;
    gpio_num_t button2_gpio_num;
    button_combined_callback_t combined_press_callback;
    button_combined_callback_t combined_long_press_callback;
    bool is_currently_combined_pressed; // Status kombinasi sedang aktif
    uint64_t combined_press_start_time_ms; // Waktu kombinasi mulai ditekan
    bool combined_long_press_triggered; // Flag untuk memastikan long press kombinasi hanya sekali pemicuan
    TimerHandle_t combined_long_press_timer; // Timer untuk long press kombinasi
} combined_button_info_t;


// --- Global Variables ---
static button_info_t s_buttons[MAX_BUTTONS];
static int s_num_buttons = 0; // Jumlah tombol yang saat ini terdaftar

static combined_button_info_t s_combined_buttons[MAX_COMBINED_BUTTONS];
static int s_num_combined_buttons = 0; // Jumlah kombinasi tombol yang saat ini terdaftar

// Queue untuk event tombol dari ISR ke task handler
typedef struct {
    gpio_num_t gpio_num;
    uint32_t level; // Level pin setelah interrupt
} button_isr_event_t;

static QueueHandle_t s_button_event_queue = NULL;
static TaskHandle_t s_button_handler_task_handle = NULL;

// --- Forward Declarations ---
static void button_isr_handler(void *arg);
static void button_handler_task(void *arg);
static void debounce_timer_callback(TimerHandle_t xTimer);
static void long_press_timer_callback(TimerHandle_t xTimer);
static void combined_long_press_timer_callback(TimerHandle_t xTimer);
static button_info_t* find_button_info(gpio_num_t gpio_num);
static combined_button_info_t* find_combined_button_info(gpio_num_t button1_gpio_num, gpio_num_t button2_gpio_num);
static void trigger_single_button_event(button_info_t *btn_info, button_event_t event);
static void trigger_combined_button_event(combined_button_info_t *combined_btn_info, button_event_t event);
static void check_and_trigger_combined_press(button_info_t *btn1_info, button_info_t *btn2_info);


// --- Implementasi Fungsi Public ---

/**
 * Menginisialisasi GPIO sebagai input dengan interrupt.
 * Membuat FreeRTOS Timers (debounce dan long press) untuk setiap tombol.
 * Membuat FreeRTOS Queue dan Task handler (jika belum ada).
 * Memasang button_isr_handler ke setiap GPIO yang relevan.
 * @param config
 * @return
 */
esp_err_t button_driver_init(const button_config_t *config) {
    if (s_num_buttons >= MAX_BUTTONS) {
        ESP_LOGE(TAG, "Max buttons reached. Cannot add more buttons.");
        return ESP_ERR_NO_MEM;
    }
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Pastikan tombol belum terdaftar
    if (find_button_info(config->gpio_num) != NULL) {
        ESP_LOGW(TAG, "Button GPIO %d already initialized.", config->gpio_num);
        return ESP_OK;
    }

    // Inisialisasi Queue dan Task hanya sekali
    if (s_button_event_queue == NULL) {
        s_button_event_queue = xQueueCreate(10, sizeof(button_isr_event_t));
        if (s_button_event_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create button event queue");
            return ESP_FAIL;
        }
    }
    if (s_button_handler_task_handle == NULL) {
        if (xTaskCreate(button_handler_task, "button_handler_task", 4096, NULL, 10, &s_button_handler_task_handle) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create button handler task");
            vQueueDelete(s_button_event_queue);
            s_button_event_queue = NULL;
            return ESP_FAIL;
        }
    }

    button_info_t *btn_info = &s_buttons[s_num_buttons];
    btn_info->gpio_num = config->gpio_num;
    btn_info->pull_mode = config->pull_mode;
    btn_info->active_level = config->active_level;
    btn_info->debounce_time_ms = config->debounce_time_ms;
    btn_info->long_press_time_ms = config->long_press_time_ms;
    btn_info->current_state = (gpio_get_level(config->gpio_num) == config->active_level) ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;
    btn_info->long_press_triggered = false;
    memset(btn_info->callbacks, 0, sizeof(btn_info->callbacks));

    // Konfigurasi GPIO
    gpio_reset_pin(config->gpio_num);
    gpio_set_direction(config->gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(config->gpio_num, config->pull_mode);
    gpio_set_intr_type(config->gpio_num, GPIO_INTR_ANYEDGE); // Interrupt pada perubahan level apapun

    // Inisialisasi Timers
    char timer_name[20];
    snprintf(timer_name, sizeof(timer_name), "db_t%d", config->gpio_num);
    btn_info->debounce_timer = xTimerCreate(timer_name, pdMS_TO_TICKS(config->debounce_time_ms), pdFALSE, (void *)config->gpio_num, debounce_timer_callback);
    if (btn_info->debounce_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create debounce timer for GPIO %d", config->gpio_num);
        return ESP_FAIL;
    }

    snprintf(timer_name, sizeof(timer_name), "lp_t%d", config->gpio_num);
    btn_info->long_press_timer = xTimerCreate(timer_name, pdMS_TO_TICKS(config->long_press_time_ms), pdFALSE, (void *)config->gpio_num, long_press_timer_callback);
    if (btn_info->long_press_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create long press timer for GPIO %d", config->gpio_num);
        xTimerDelete(btn_info->debounce_timer, 0);
        return ESP_FAIL;
    }

    // Pasang ISR ke GPIO
    gpio_isr_handler_add(config->gpio_num, button_isr_handler, (void *)config->gpio_num);

    s_num_buttons++;


    const char *pull_mode_str;
    if ((int)config->pull_mode == (int)GPIO_PULLUP_ENABLE) { // Lakukan casting ke int
        pull_mode_str = "PULL_UP";
    } else if ((int)config->pull_mode == (int)GPIO_PULLDOWN_ENABLE) { // Lakukan casting ke int
        pull_mode_str = "PULL_DOWN";
    } else {
        pull_mode_str = "FLOATING";
    }

    ESP_LOGI(TAG, "Button driver initialized for GPIO %d (Active Level: %d, Pull: %s, Debounce: %dms, Long Press: %dms)",
             config->gpio_num, config->active_level, pull_mode_str,
             config->debounce_time_ms, config->long_press_time_ms);

    return ESP_OK;
}

esp_err_t button_driver_register_callback(gpio_num_t gpio_num, button_event_t event, button_callback_t callback) {
    if (event >= (BUTTON_EVENT_LONG_PRESS + 1) || event == BUTTON_EVENT_NONE || event == BUTTON_EVENT_COMBINED_PRESS || event == BUTTON_EVENT_COMBINED_LONG_PRESS) {
        ESP_LOGE(TAG, "Invalid event type %d for single button callback.", event);
        return ESP_ERR_INVALID_ARG;
    }

    button_info_t *btn_info = find_button_info(gpio_num);
    if (btn_info == NULL) {
        ESP_LOGE(TAG, "Button GPIO %d not initialized.", gpio_num);
        return ESP_ERR_NOT_FOUND;
    }

    btn_info->callbacks[event] = callback;
    ESP_LOGD(TAG, "Callback registered for GPIO %d, Event %d", gpio_num, event);
    return ESP_OK;
}

esp_err_t button_driver_unregister_callback(gpio_num_t gpio_num, button_event_t event) {
    if (event >= (BUTTON_EVENT_LONG_PRESS + 1) || event == BUTTON_EVENT_NONE || event == BUTTON_EVENT_COMBINED_PRESS || event == BUTTON_EVENT_COMBINED_LONG_PRESS) {
        ESP_LOGE(TAG, "Invalid event type %d for single button callback.", event);
        return ESP_ERR_INVALID_ARG;
    }

    button_info_t *btn_info = find_button_info(gpio_num);
    if (btn_info == NULL) {
        ESP_LOGE(TAG, "Button GPIO %d not initialized.", gpio_num);
        return ESP_ERR_NOT_FOUND;
    }

    btn_info->callbacks[event] = NULL; // Set callback ke NULL
    ESP_LOGD(TAG, "Callback unregistered for GPIO %d, Event %d", gpio_num, event);
    return ESP_OK;
}

button_state_t button_driver_get_state(gpio_num_t gpio_num) {
    button_info_t *btn_info = find_button_info(gpio_num);
    if (btn_info == NULL) {
        ESP_LOGW(TAG, "Button GPIO %d not initialized, returning RELEASED.", gpio_num);
        return BUTTON_STATE_RELEASED;
    }
    return btn_info->current_state;
}


esp_err_t button_driver_register_combined_callback(gpio_num_t button1_gpio_num, gpio_num_t button2_gpio_num, button_event_t event, button_combined_callback_t callback) {
    if ((event != BUTTON_EVENT_COMBINED_PRESS && event != BUTTON_EVENT_COMBINED_LONG_PRESS) || callback == NULL) {
        ESP_LOGE(TAG, "Invalid event type or NULL callback for combined button.");
        return ESP_ERR_INVALID_ARG;
    }
    if (button1_gpio_num == button2_gpio_num) {
        ESP_LOGE(TAG, "Cannot register combined callback for the same GPIO: %d", button1_gpio_num);
        return ESP_ERR_INVALID_ARG;
    }

    // Pastikan kedua tombol individual sudah diinisialisasi
    if (find_button_info(button1_gpio_num) == NULL || find_button_info(button2_gpio_num) == NULL) {
        ESP_LOGE(TAG, "One or both buttons (%d, %d) for combined callback are not initialized.", button1_gpio_num, button2_gpio_num);
        return ESP_ERR_NOT_FOUND;
    }

    // Cari entri kombinasi yang sudah ada atau buat yang baru
    combined_button_info_t *combined_btn_info = find_combined_button_info(button1_gpio_num, button2_gpio_num);
    if (combined_btn_info == NULL) {
        if (s_num_combined_buttons >= MAX_COMBINED_BUTTONS) {
            ESP_LOGE(TAG, "Max combined buttons reached. Cannot add more combinations.");
            return ESP_ERR_NO_MEM;
        }
        combined_btn_info = &s_combined_buttons[s_num_combined_buttons];
        combined_btn_info->button1_gpio_num = button1_gpio_num;
        combined_btn_info->button2_gpio_num = button2_gpio_num;
        combined_btn_info->is_currently_combined_pressed = false;
        combined_btn_info->combined_long_press_triggered = false;
        combined_btn_info->combined_press_callback = NULL; // Inisialisasi null
        combined_btn_info->combined_long_press_callback = NULL; // Inisialisasi null

        // Buat timer long press kombinasi jika belum ada
        char timer_name[30];
        snprintf(timer_name, sizeof(timer_name), "comb_lp_t%d_%d", button1_gpio_num, button2_gpio_num);
        // Gunakan long press time dari tombol pertama sebagai default, bisa diatur lebih lanjut jika perlu
        button_info_t *btn1 = find_button_info(button1_gpio_num);
        if (btn1) {
            combined_btn_info->combined_long_press_timer = xTimerCreate(timer_name, pdMS_TO_TICKS(btn1->long_press_time_ms), pdFALSE, (void *)combined_btn_info, combined_long_press_timer_callback);
            if (combined_btn_info->combined_long_press_timer == NULL) {
                ESP_LOGE(TAG, "Failed to create combined long press timer for %d, %d", button1_gpio_num, button2_gpio_num);
                return ESP_FAIL;
            }
        } else { // Seharusnya tidak terjadi karena sudah dicek di awal
             ESP_LOGE(TAG, "Internal error: button info not found for combined timer creation.");
             return ESP_FAIL;
        }

        s_num_combined_buttons++;
        ESP_LOGI(TAG, "New combined button pair registered: %d & %d", button1_gpio_num, button2_gpio_num);
    }

    if (event == BUTTON_EVENT_COMBINED_PRESS) {
        combined_btn_info->combined_press_callback = callback;
    }

    if (event == BUTTON_EVENT_COMBINED_LONG_PRESS) {
        combined_btn_info->combined_long_press_callback = callback;
    }
    ESP_LOGD(TAG, "Combined callback registered for %d & %d, Event %d", button1_gpio_num, button2_gpio_num, event);
    return ESP_OK;
}

esp_err_t button_driver_unregister_combined_callback(gpio_num_t button1_gpio_num, gpio_num_t button2_gpio_num, button_event_t event) {
     if (event != BUTTON_EVENT_COMBINED_PRESS && event != BUTTON_EVENT_COMBINED_LONG_PRESS) {
        ESP_LOGE(TAG, "Invalid event type for combined button unregister.");
        return ESP_ERR_INVALID_ARG;
    }

    combined_button_info_t *combined_btn_info = find_combined_button_info(button1_gpio_num, button2_gpio_num);
    if (combined_btn_info == NULL) {
        ESP_LOGW(TAG, "Combined button pair (%d, %d) not found.", button1_gpio_num, button2_gpio_num);
        return ESP_ERR_NOT_FOUND;
    }

    if (event == BUTTON_EVENT_COMBINED_PRESS) {
        combined_btn_info->combined_press_callback = NULL;
    }

    if (event == BUTTON_EVENT_COMBINED_LONG_PRESS) {
        combined_btn_info->combined_long_press_callback = NULL;
    }
    ESP_LOGD(TAG, "Combined callback unregistered for %d & %d, Event %d", button1_gpio_num, button2_gpio_num, event);

    // Optional: Jika kedua callback kombinasi sudah NULL, Anda bisa menghapus entri kombinasi dari array
    // Ini akan membutuhkan pemindahan elemen di array s_combined_buttons untuk menjaga kerapatan.
    // Untuk kesederhanaan, kita biarkan saja entri NULL di sini.
    return ESP_OK;
}


// --- Implementasi Fungsi Internal/Static ---

static button_info_t* find_button_info(gpio_num_t gpio_num) {
    for (int i = 0; i < s_num_buttons; i++) {
        if (s_buttons[i].gpio_num == gpio_num) {
            return &s_buttons[i];
        }
    }
    return NULL;
}

static combined_button_info_t* find_combined_button_info(gpio_num_t button1_gpio_num, gpio_num_t button2_gpio_num) {
    for (int i = 0; i < s_num_combined_buttons; i++) {
        // Cek kedua urutan karena pendaftaran bisa button1, button2 atau button2, button1
        if ((s_combined_buttons[i].button1_gpio_num == button1_gpio_num && s_combined_buttons[i].button2_gpio_num == button2_gpio_num) ||
            (s_combined_buttons[i].button1_gpio_num == button2_gpio_num && s_combined_buttons[i].button2_gpio_num == button1_gpio_num)) {
            return &s_combined_buttons[i];
        }
    }
    return NULL;
}

static void trigger_single_button_event(button_info_t *btn_info, button_event_t event) {
    if (btn_info && btn_info->callbacks[event]) {
        btn_info->callbacks[event](btn_info->gpio_num, event);
    }
}

static void trigger_combined_button_event(combined_button_info_t *combined_btn_info, button_event_t event) {
    if (combined_btn_info) {
        if (event == BUTTON_EVENT_COMBINED_PRESS && combined_btn_info->combined_press_callback) {
            combined_btn_info->combined_press_callback(combined_btn_info->button1_gpio_num, combined_btn_info->button2_gpio_num, event);
        } else if (event == BUTTON_EVENT_COMBINED_LONG_PRESS && combined_btn_info->combined_long_press_callback) {
            combined_btn_info->combined_long_press_callback(combined_btn_info->button1_gpio_num, combined_btn_info->button2_gpio_num, event);
        }
    }
}

// ISR untuk menangani perubahan GPIO
static void IRAM_ATTR button_isr_handler(void *arg) {
    gpio_num_t gpio_num = (gpio_num_t)arg;
    button_isr_event_t event = {
        .gpio_num = gpio_num,
        .level = gpio_get_level(gpio_num)
    };
    // Kirim event ke queue dari ISR
    xQueueSendFromISR(s_button_event_queue, &event, NULL);
}

// Task untuk memproses event tombol dari queue
static void button_handler_task(void *arg) {
    button_isr_event_t event;
    while (1) {
        if (xQueueReceive(s_button_event_queue, &event, portMAX_DELAY)) {
            button_info_t *btn_info = find_button_info(event.gpio_num);
            if (btn_info == NULL) {
                ESP_LOGW(TAG, "Event for uninitialized GPIO %d received.", event.gpio_num);
                continue;
            }

            // Hentikan timer debounce yang sedang berjalan jika ada
            xTimerStop(btn_info->debounce_timer, 0);

            // Mulai timer debounce
            // Callback timer akan dipanggil setelah debounce_time_ms
            xTimerStart(btn_info->debounce_timer, 0);
        }
    }
}

// Callback setelah debounce selesai
static void debounce_timer_callback(TimerHandle_t xTimer) {
    gpio_num_t gpio_num = (gpio_num_t)pvTimerGetTimerID(xTimer);
    button_info_t *btn_info = find_button_info(gpio_num);
    if (btn_info == NULL) {
        ESP_LOGE(TAG, "Debounce timer callback for unknown GPIO %d", gpio_num);
        return;
    }

    uint32_t current_level = gpio_get_level(gpio_num);
    button_state_t new_state = (current_level == btn_info->active_level) ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;

    if (new_state != btn_info->current_state) {
        // State tombol benar-benar berubah setelah debounce
        btn_info->current_state = new_state;

        if (new_state == BUTTON_STATE_PRESSED) {
            btn_info->press_start_time_ms = esp_timer_get_time() / 1000; // Simpan waktu tekan
            btn_info->long_press_triggered = false; // Reset flag long press
            xTimerStart(btn_info->long_press_timer, 0); // Mulai timer long press

            trigger_single_button_event(btn_info, BUTTON_EVENT_PRESS);
            ESP_LOGD(TAG, "Button %d PRESSED", gpio_num);
        } else { // BUTTON_STATE_RELEASED
            xTimerStop(btn_info->long_press_timer, 0); // Hentikan timer long press

            trigger_single_button_event(btn_info, BUTTON_EVENT_RELEASE);
            ESP_LOGD(TAG, "Button %d RELEASED", gpio_num);

            // Cek untuk CLICK event (jika long press belum terpicu)
            if (!btn_info->long_press_triggered) {
                trigger_single_button_event(btn_info, BUTTON_EVENT_CLICK);
                ESP_LOGD(TAG, "Button %d CLICK", gpio_num);
            }
        }

        // Setelah state tombol berubah, cek semua kombinasi yang melibatkan tombol ini
        for (int i = 0; i < s_num_combined_buttons; i++) {
            combined_button_info_t *combined_btn_info = &s_combined_buttons[i];
            button_info_t *btn1 = find_button_info(combined_btn_info->button1_gpio_num);
            button_info_t *btn2 = find_button_info(combined_btn_info->button2_gpio_num);

            if (!btn1 || !btn2) continue; // Should not happen if initialization is correct

            // Jika tombol yang baru saja berubah adalah salah satu dari pasangan kombinasi
            if (btn_info->gpio_num == btn1->gpio_num || btn_info->gpio_num == btn2->gpio_num) {
                check_and_trigger_combined_press(btn1, btn2);
            }
        }
    }
}

// Callback untuk deteksi long press tombol tunggal
static void long_press_timer_callback(TimerHandle_t xTimer) {
    gpio_num_t gpio_num = (gpio_num_t)pvTimerGetTimerID(xTimer);
    button_info_t *btn_info = find_button_info(gpio_num);
    if (btn_info == NULL) {
        ESP_LOGE(TAG, "Long press timer callback for unknown GPIO %d", gpio_num);
        return;
    }

    // Pastikan tombol masih ditekan saat timer berakhir
    if (btn_info->current_state == BUTTON_STATE_PRESSED && !btn_info->long_press_triggered) {
        btn_info->long_press_triggered = true;
        trigger_single_button_event(btn_info, BUTTON_EVENT_LONG_PRESS);
        ESP_LOGI(TAG, "Button %d LONG PRESS", gpio_num);
    }
}

// Fungsi untuk memeriksa dan memicu event kombinasi
static void check_and_trigger_combined_press(button_info_t *btn1_info, button_info_t *btn2_info) {
    if (!btn1_info || !btn2_info) return;

    combined_button_info_t *combined_btn_info = find_combined_button_info(btn1_info->gpio_num, btn2_info->gpio_num);
    if (combined_btn_info == NULL) return; // Tidak ada kombinasi yang terdaftar untuk pasangan ini

    // Deteksi kombinasi PRESS
    if (btn1_info->current_state == BUTTON_STATE_PRESSED && btn2_info->current_state == BUTTON_STATE_PRESSED) {
        if (!combined_btn_info->is_currently_combined_pressed) {
            // Kombinasi baru saja terjadi
            combined_btn_info->is_currently_combined_pressed = true;
            combined_btn_info->combined_press_start_time_ms = esp_timer_get_time() / 1000;
            combined_btn_info->combined_long_press_triggered = false; // Reset flag long press kombinasi

            // Mulai timer long press kombinasi
            xTimerStart(combined_btn_info->combined_long_press_timer, 0);

            trigger_combined_button_event(combined_btn_info, BUTTON_EVENT_COMBINED_PRESS);
            ESP_LOGI(TAG, "Combined Button (%d, %d) PRESSED!", btn1_info->gpio_num, btn2_info->gpio_num);
        }
    } else { // Salah satu atau kedua tombol dilepas
        if (combined_btn_info->is_currently_combined_pressed) {
            // Kombinasi baru saja berakhir
            combined_btn_info->is_currently_combined_pressed = false;
            xTimerStop(combined_btn_info->combined_long_press_timer, 0); // Hentikan timer long press kombinasi
            ESP_LOGI(TAG, "Combined Button (%d, %d) RELEASED!", btn1_info->gpio_num, btn2_info->gpio_num);
            // Anda bisa tambahkan event BUTTON_EVENT_COMBINED_RELEASE jika diperlukan
        }
    }
}

// Callback untuk deteksi long press kombinasi tombol
static void combined_long_press_timer_callback(TimerHandle_t xTimer) {
    combined_button_info_t *combined_btn_info = (combined_button_info_t *)pvTimerGetTimerID(xTimer);
    if (combined_btn_info == NULL) {
        ESP_LOGE(TAG, "Combined long press timer callback for unknown combined button info.");
        return;
    }

    // Pastikan kedua tombol masih ditekan saat timer berakhir
    button_info_t *btn1 = find_button_info(combined_btn_info->button1_gpio_num);
    button_info_t *btn2 = find_button_info(combined_btn_info->button2_gpio_num);

    if (btn1 && btn2 &&
        btn1->current_state == BUTTON_STATE_PRESSED &&
        btn2->current_state == BUTTON_STATE_PRESSED &&
        !combined_btn_info->combined_long_press_triggered) {

        combined_btn_info->combined_long_press_triggered = true;
        trigger_combined_button_event(combined_btn_info, BUTTON_EVENT_COMBINED_LONG_PRESS);
        ESP_LOGI(TAG, "Combined Button (%d, %d) LONG PRESS!", combined_btn_info->button1_gpio_num, combined_btn_info->button2_gpio_num);
    }
}