//
// Created by Human Race on 27/05/2025.
//

#include "comm_task.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"


static const char* TAG = "COMM_TASK";

uint8_t receiver_mac[ESP_NOW_ETH_ALEN] = { 0x34, 0x98, 0x7A, 0x89, 0x89, 0x08 };

// queuehandler
QueueHandle_t comm_task_rcv_queue = NULL;
QueueHandle_t comm_task_send_queue = NULL;

// forward declaration
static void esp_now_send_cb(const uint8_t* mac_address, esp_now_send_status_t status);
static void esp_now_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len);

esp_err_t comm_task_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "ESP WIFI_MODE_STA");

  // --- KRUSIAL: INISIALISASI ESP-NOW DI SINI ---
  esp_err_t esp_now_init_ret = esp_now_init();
  if (esp_now_init_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize ESP-NOW: %s", esp_err_to_name(esp_now_init_ret));
    return esp_now_init_ret; // Kembalikan error jika inisialisasi ESP-NOW gagal
  }
  ESP_LOGI(TAG, "ESP-NOW initialized.");

  // tambahkan peer (penerima)
  esp_now_peer_info_t peer_info = {};
  memcpy(peer_info.peer_addr, receiver_mac, ESP_NOW_ETH_ALEN);
  peer_info.channel = 0; // channel wifi saat ini
  peer_info.encrypt = false; // Tanpa enkripsi
  peer_info.ifidx = WIFI_IF_STA; // jika sender dalam mode STA

  // daftarkan callback
  ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb)); // untuk status pengiriman
  ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb)); // untuk menerima data

  ESP_LOGI(TAG, "ADDING PEER: %02x:%02x:%02x:%02x:%02x:%02x",
    receiver_mac[0], receiver_mac[1], receiver_mac[2], receiver_mac[3],
    receiver_mac[4], receiver_mac[5]);

  // check peer
  if (!esp_now_is_peer_exist(receiver_mac)) {
    esp_err_t add_peer_ret = esp_now_add_peer(&peer_info);
    if (add_peer_ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(add_peer_ret));
      return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Successfully added PEER");
  } else {
    ESP_LOGI(TAG, "PEER already exist");
  }

  return ESP_OK;
}

bool comm_task_rcv_from_main_queue(QueueHandle_t main_to_comm_queue) {
  if (main_to_comm_queue == NULL) return false;
  comm_task_rcv_queue = main_to_comm_queue;
  return true;
}

bool comm_task_send_to_main_queue(QueueHandle_t comm_to_main_queue) {
  if (comm_to_main_queue == NULL) return false;
  comm_task_send_queue = comm_to_main_queue;
  return true;
}

void comm_task_update(void) {
  while (1) {
    comm_send_data_t comm_send_data;
    // menerima dari task lain
    if (xQueueReceive(comm_task_rcv_queue, &comm_send_data, pdMS_TO_TICKS(200)) == pdPASS) {
      // mengirim data ke esp32_A
      esp_err_t send_ret = esp_now_send(receiver_mac, (uint8_t *) &comm_send_data, sizeof(comm_send_data));
      if (send_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(send_ret));
      } else {
        ESP_LOGI(TAG, "Successfully sent data to ESP32 B");
      }
    } else {
      ESP_LOGW(TAG, "Failed to receive data from comm_task_rcv_queue");
    }

    vTaskDelay(pdMS_TO_TICKS(100));

  }
}

static void esp_now_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
  if (mac_addr == NULL) {
    ESP_LOGE(TAG, "Send callback: MAC address is null");
    return;
  }
  ESP_LOGI(TAG, "Sent to %02X:%02X:%02X:%02X:%02X:%02X status: %s",
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
    status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

static void esp_now_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (mac_addr == NULL) {
    ESP_LOGE(TAG, "mac_addr is NULL");
    return;
  }

  if (data == NULL) {
    ESP_LOGE(TAG, "weight_data is NULL");
    return;
  }

  if (data_len != sizeof(weight_data_t)) {
    ESP_LOGE(TAG, "Received data length (%d) does not match expected size (%d) for weight_data_t",
             data_len, sizeof(weight_data_t));
    return;
  }

  weight_data_t buffer_weight;
  memcpy(&buffer_weight, data, sizeof(weight_data_t));

  // kirim queue
  if (xQueueSendFromISR(comm_task_send_queue, &buffer_weight, NULL) != pdPASS) {
    ESP_LOGE(TAG, "Failed to send comm_task_queue");
  } else {
    ESP_LOGI(TAG, "Successfully sent comm_task_queue");
  }
}