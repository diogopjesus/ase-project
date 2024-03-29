#pragma once
#include "driver/spi_master.h"

esp_err_t spi_25LC040_init(spi_host_device_t masterHostId, int csPin, int sckPin, int mosiPin, int misoPin,
                           int clkSpeedHz, spi_device_handle_t *pDevHandle);

esp_err_t spi_25LC040_free(spi_host_device_t masterHostId, spi_device_handle_t devHandle);

esp_err_t spi_25LC040_read_byte(spi_device_handle_t devHandle,
                                uint16_t address, uint8_t *pData);

esp_err_t spi_25LC040_write_byte(spi_device_handle_t devHandle,
                                 uint16_t address, uint8_t data);

esp_err_t spi_25LC040_write_page(spi_device_handle_t devHandle,
                                 uint16_t address, const uint8_t *pBuffer, uint8_t size);

esp_err_t spi_25LC040_write_enable(spi_device_handle_t devHandle);

esp_err_t spi_25LC040_write_disable(spi_device_handle_t devHandle);

esp_err_t spi_25LC040_read_status(spi_device_handle_t devHandle, uint8_t *pStatus);

esp_err_t spi_25LC040_write_status(spi_device_handle_t devHandle, uint8_t status);
