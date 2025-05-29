//
// Created by Human Race on 28/05/2025.
//

#include "lcd_driver.h"
#include "LiquidCrystal_I2C.h"

static const char *TAG = "LCD_DRIVER";

#ifdef __cplusplus
extern "C" {
#endif

lcd_handle_t liquidcrystal_i2c_create(uint8_t lcd_addr, uint8_t cols, uint8_t rows) {

  LiquidCrystal_I2C* lcd_ptr = new LiquidCrystal_I2C(lcd_addr, cols, rows);

  return (lcd_handle_t)lcd_ptr;
}

void liquidcrystal_i2c_init(lcd_handle_t lcd_handle) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    lcd->init();
  }
}

void lcd_backlight(lcd_handle_t lcd_handle) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    lcd->backlight();
  }
}

void lcd_no_backlight(lcd_handle_t lcd_handle) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    lcd->noBacklight();
  }
}

void lcd_set_cursor(lcd_handle_t lcd_handle, uint8_t col, uint8_t row) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    lcd->setCursor(col, row);
  }
}

void lcd_clear(lcd_handle_t lcd_handle) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    lcd->clear();
  }
}

void lcd_print(lcd_handle_t lcd_handle, char* str) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    lcd->print(str);
  }
}

void lcd_deinit(lcd_handle_t lcd_handle) {
  LiquidCrystal_I2C* lcd = static_cast<LiquidCrystal_I2C*>(lcd_handle);
  if (lcd) {
    delete lcd;
  }
}

#ifdef __cplusplus
}
#endif