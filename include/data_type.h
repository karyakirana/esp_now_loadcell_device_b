//
// Created by Human Race on 26/05/2025.
//

#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include <stdint.h>

typedef enum {
  NORMAL_MODE,
  NORMAL_MODE_TARE,
  CAL_INIT_MODE,
  CAL_WAITING_MODE,
  CAL_INPUT_MODE,
  CAL_CONFIRMATION_MODE,
  CAL_CANCEL_MODE,
  SLEEP_MODE,
  WAKE_UP_MODE,
} main_state_t;

typedef enum {
  CMD_NORMAL,
  CMD_NORMAL_TARE,
  CMD_CAL_INIT,
  CMD_CAL_WAITING,
  CMD_CAL_INPUT,
  CMD_CAL_CONFIRMATION,
  CMD_CAL_CANCEL,
  CMD_SLEEP,
  CMD_WAKE_UP,
  CMD_UNKNOWN_OR_INVALID
} cmd_main_t;

typedef struct {
  main_state_t  main_state;
  float         filtered_weight;
  float         units;
  long          raw_weight;
  bool          is_ready;
} weight_data_t;

typedef struct {
  const char* line_1;
  const char* line_2;
} led_data_t;

// ESP32 B akan mengisi struct ini dan mengirimkannya ke A
typedef struct {
  cmd_main_t  command;
  float       value;
} comm_send_data_t;

#endif //DATA_TYPE_H
