//
// Created by Human Race on 28/05/2025.
//

#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <mine_header.h>
#include "driver/i2c.h"

typedef void* lcd_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

lcd_handle_t liquidcrystal_i2c_create(uint8_t lcd_addr, uint8_t cols, uint8_t rows);

void liquidcrystal_i2c_init(lcd_handle_t lcd_handle);

void lcd_backlight(lcd_handle_t lcd_handle);

void lcd_no_backlight(lcd_handle_t lcd_handle);

void lcd_set_cursor(lcd_handle_t lcd_handle, uint8_t col, uint8_t row);

void lcd_clear(lcd_handle_t lcd_handle);

void lcd_print(lcd_handle_t lcd_handle, char* str);

void lcd_deinit(lcd_handle_t lcd_handle);

#ifdef __cplusplus
}
#endif

#endif //LCD_DRIVER_H
