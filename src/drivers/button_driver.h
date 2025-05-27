//
// Created by Human Race on 26/05/2025.
//

#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h> // Untuk TickType_t
#include <button_defs.h>

/**
 * @brief Pointer ke fungsi callback untuk event tombol
 * @param button_gpio_num Nomor GPIO tombol yang memicu event
 * @param event Event yang terjadi
 */
typedef void (*button_callback_t)(gpio_num_t button_gpio_num, button_event_t event);

/**
 *
 * @brief Pointer ke fungsi callback untuk event tombol
 * @param button1_gpio_num Nomor GPIO tombol 1 yang memicu event
 * @param button2_gpio_num Nomor GPIO tombol 2 yang memicu event
 * @param event Event yang terjadi
 */
typedef void (*button_combined_callback_t)(gpio_num_t button1_gpio_num,
                                            gpio_num_t button2_gpio_num,
                                            button_event_t event);


/**
 * @brief Menginisialisasi tombol
 * @param config Pointer ke struktur konfigurasi tombol
 * @return ESP_OK jika berhasil, atau kode error ESP-IDF lainnya
 */
esp_err_t button_driver_init(const button_config_t *config);

/**
 * @brief Memasang callback untuk event tombol.
 * @param gpio_num Nomor GPIO tombol
 * @param event Event yang ingin dipantau
 * @param callback Fungsi callback yang akan dipanggil saat event terjadi
 * @return ESP_OK jika berhasil, atau kode error ESP-IDF lainnya
 */
esp_err_t button_driver_register_callback(gpio_num_t gpio_num, button_event_t event, button_callback_t callback);

/**
 * @brief Melepas callback dari event tombol
 * @param gpio_num Nomor GPIO tombol
 * @param event Event yang ingin dipantau
 * @return ESP_OK jika berhasil, atau kode error ESP-IDF lainnya
 */
esp_err_t button_driver_unregister_callback(gpio_num_t gpio_num, button_event_t event);

/**
 *
 * @param gpio_num
 * @return
 */
button_state_t button_driver_get_state(gpio_num_t gpio_num);

/**
 * @brief Mendapatkan status tombol saat ini.
 * @param button1_gpio_num
 * @param button2_gpio_num
 * @param event
 * @param callback
 * @return
 */
esp_err_t button_driver_register_combined_callback(
                                                    gpio_num_t button1_gpio_num,
                                                    gpio_num_t button2_gpio_num,
                                                    button_event_t event,
                                                    button_combined_callback_t callback);

/**
 * @brief Melepas callback dari event tombol
 * @param button1_gpio_num
 * @param button2_gpio_num
 * @param event
 * @return
 */
esp_err_t button_driver_unregister_combined_callback(
                                                      gpio_num_t button1_gpio_num,
                                                      gpio_num_t button2_gpio_num,
                                                      button_event_t event);


#endif //BUTTON_DRIVER_H
