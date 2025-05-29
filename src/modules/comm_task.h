//
// Created by Human Race on 27/05/2025.
//

#ifndef COMM_TASK_H
#define COMM_TASK_H

#include <mine_header.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t comm_task_init();

bool comm_task_rcv_from_main_queue(QueueHandle_t main_to_comm_queue);

bool comm_task_send_to_main_queue(QueueHandle_t comm_to_main_queue);

void comm_task_update(void);

#ifdef __cplusplus
}
#endif

#endif //COMM_TASK_H
