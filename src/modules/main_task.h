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

bool main_change_state(main_state_t *main_state);

bool main_queue_rcv_comm(QueueHandle_t comm_queue, weight_data_t weight_data);

bool main_queue_rcv_btn(void);

bool main_queue_send_comm(QueueHandle_t main_to_comm_queue, comm_send_data_t *comm_send_data);

bool main_queue_send_led(QueueHandle_t main_to_led_queue, led_data_t *led_data);

#ifdef __cplusplus
}
#endif

#endif //MAIN_TASK_H
