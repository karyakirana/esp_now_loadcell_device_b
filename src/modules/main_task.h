//
// Created by Human Race on 26/05/2025.
//

#ifndef MAIN_TASK_H
#define MAIN_TASK_H

#include <mine_header.h>
#include <button_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

void main_task_init(void);

bool main_task_rcv_button(QueueHandle_t rcv_btn_queue);

bool main_task_rcv_comm(QueueHandle_t rcv_comm_queue);

bool main_task_send_comm(QueueHandle_t send_comm_queue);

bool main_task_send_oled(QueueHandle_t send_oled_queue);

void main_task_update(void);

#ifdef __cplusplus
}
#endif

#endif //MAIN_TASK_H
