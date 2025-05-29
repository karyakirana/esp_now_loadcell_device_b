//
// Created by Human Race on 28/05/2025.
//

#include "lcd_task.h"
#include "driver/i2c.h"
#include "drivers/lcd_driver.h"

static const char* TAG = "LCD_TASK";

// Definisi PIN I2C Anda

// Alamat I2C LCD Anda (biasanya 0x27 atau 0x3F)
// Anda mungkin perlu mencoba kedua alamat ini jika tidak yakin
#define LCD_I2C_ADDR                0x27

lcd_handle_t lcd_handle = NULL;
QueueHandle_t lcd_queue_from_main = NULL;

lcd_state current_lcd_state = LCD_IDLE;

led_data_t lcd_data;

// forward declaration
static void lcd_state_dispatcher();

static void lcd_render(void);
static void lcd_render_init(void);
static void lcd_render_normal(void);
static void lcd_render_tare(void);
static void lcd_render_calibration(void);
static void lcd_render_calibration_waiting(void);
static void lcd_render_calibration_input(void);
static void lcd_render_confirmation(void);

void lcd_task_init(void) {
  lcd_handle = liquidcrystal_i2c_create(LCD_I2C_ADDR, 16, 2);
  liquidcrystal_i2c_init(lcd_handle);
  lcd_backlight(lcd_handle);
}

bool lcd_queue_handler_from_main(QueueHandle_t main_to_lcd_queue) {
  if (main_to_lcd_queue == NULL) return false;
  lcd_queue_from_main = main_to_lcd_queue;
  return true;
}

void lcd_task_update(void) {
  while (1) {
    if (xQueueReceive(lcd_queue_from_main, &lcd_data, pdMS_TO_TICKS(200)) != pdPASS) {
      ESP_LOGW(TAG, "LCD DATA receive failed");
    } else {
      ESP_LOGI(TAG, "LCD DATA line 1 %s", lcd_data.line_1);
      ESP_LOGI(TAG, "LCD DATA receive success");
      lcd_render();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// --- static function ---
static void lcd_state_dispatcher(void) {
  switch (current_lcd_state) {
    case LCD_IDLE:
      break;
    case LCD_INIT:
      lcd_render_init();
      break;
    case LCD_NORMAL:
      lcd_render_normal();
      break;
    case LCD_CALIBRATION:
      lcd_render_calibration();
      break;
    case LCD_CALIBRATION_WAITING:
      lcd_render_calibration_waiting();
      break;
    case LCD_CALIBRATION_INPUT:
      lcd_render_calibration_input();
      break;
    case LCD_CONFIRMATION:
      lcd_render_confirmation();
      break;
    default:
      break;
  }
}

static void lcd_render(void) {
  char buffer_line_1[16];
  char buffer_line_2[16];

  const char* prev_line2  = "";

  lcd_set_cursor(lcd_handle, 0, 0);
  if (strlen(lcd_data.line_1) <= 16) {
    strncpy(buffer_line_1, lcd_data.line_1, 16);
    lcd_print(lcd_handle, buffer_line_1);
  }

  lcd_set_cursor(lcd_handle, 0, 1);
  if (strlen(lcd_data.line_2) <= 16) {
    if (prev_line2 != lcd_data.line_2) {
      // kosongkan dulu
      lcd_print(lcd_handle, "                ");
      prev_line2 = lcd_data.line_2;
    }
    strncpy(buffer_line_2, lcd_data.line_2, 16);
    lcd_print(lcd_handle, buffer_line_2);
  }
}

static void lcd_render_init(void) {}
static void lcd_render_normal(void) {}
static void lcd_render_tare(void) {}
static void lcd_render_calibration(void) {}
static void lcd_render_calibration_waiting(void) {}
static void lcd_render_calibration_input(void) {}
static void lcd_render_confirmation(void) {}