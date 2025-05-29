//
// Created by Human Race on 28/05/2025.
//

#ifndef LCD_TASK_H
#define LCD_TASK_H

#include <mine_header.h>

#ifdef __cplusplus
extern "C" {
#endif

void lcd_task_init(void);

bool lcd_queue_handler_from_main(QueueHandle_t queue);

void lcd_task_update(void);

#ifdef __cplusplus
}
#endif

#endif //LCD_TASK_H
