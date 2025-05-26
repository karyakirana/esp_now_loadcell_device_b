//
// Created by Human Race on 26/05/2025.
//

#ifndef BUTTON_DEFS_H
#define BUTTON_DEFS_H

// 1. Tipe Event Tombol yang akan dihasilkan
typedef enum {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_PRESSED,
  BUTTON_EVENT_RELEASED,
  BUTTON_EVENT_CLICK,
  BUTTON_EVENT_LONG_PRESS,
  BUTTON_EVENT_DOUBLE_CLICK,
  BUTTON_EVENT_DOUBLE_LONG_PRESS,
} button_event_type_t;

// 2. Internal state untuk setiap tombol
typedef enum {
  BTN_STATE_IDLE = 0,                   // Tombol tidak ditekan, siap untuk tekan pertama
  BTN_STATE_PRESSED,                    // Tombol ditekan, menunggu lepas atau long press
  BTN_STATE_LONG_PRESS_DETECTED,        // Tombol ditekan cukup lama, long press sudah dipicu
  BTN_STATE_WAITING_FOR_SECOND_PRESS,   // Tombol dilepas, menunggu tekan kedua untuk double click
  BTN_STATE_DOUBLE_CLICK_DETECTED,      // Tekan kedua diterima, double click sudah dipicu
  BTN_STATE_DOUBLE_LONG_PRESS_DETECTED, // Tekan kedua diterima, double long press sudah dipicu
} button_internal_state_t;

// 3. Konfigurasi waktu (ms)
#define BUTTON_DEBOUNCE_MS                50
#define BUTTON_LONG_PRESS_TIME_MS         1000
#define BUTTON_DOUBLE_CLICK_TIMEOUT_MS    300
#define BUTTON_DOUBLE_LONG_PRESS_TIME_MS  2000

// 4. Struktur Data untuk setiap tombol
typedef struct {
  uint8_t gpio_num;
  button_internal_state_t current_state;
  TickType_t last_press_tick;
  TickType_t last_release_tick;
  int press_count;
  bool is_long_press_active;
  bool is_double_long_press_active;
} button_t;

// 5. Struktur Event Tombol yang Akan Dikirim
typedef struct {
  uint8_t gpio_num;
  button_internal_state_t event_type;
} button_event_t;

#endif //BUTTON_DEFS_H
