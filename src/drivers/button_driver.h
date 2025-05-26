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
 * @brief Menginisialisasi driver tombol.
 * Membuat queue event tombol dan task untuk polling tombol.
 * @param num_buttons Jumlah tombol yang akan diinisialisasi.
 * @param button_gpios Array nomor GPIO untuk setiap tombol.
 * @param event_queue_size Ukutan Queue untuk event tombol.
 * @return true jika berhasil, false jika gagal
 */
bool button_driver_init(uint8_t num_buttons, const uint8_t button_gpios[], uint32_t event_queue_size);

/**
 * @brief Mendapatkan event tombol dari queue.
 * @param event Pointer ke struktur button_event_t untuk menyimpan event yang diterima.
 * @param timeouts_ms Waktu untuk tunggu maksimum dalam milidetik.
 * @return true jika event diterima, false jika timeout.
 */
bool button_driver_get_event(button_event_t *event, TickType_t timeouts_ms);

#endif //BUTTON_DRIVER_H
