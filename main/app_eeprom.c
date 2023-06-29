#include <stdio.h>

#include "app_eeprom.h"

static const char *TAG = "ASE-PROJECT-EEPROM";

static uint16_t eeprom_address = 0x0000;

void eeprom_init(spi_device_handle_t *devHandle)
{
    ESP_ERROR_CHECK(spi_25LC040_init(SPI_MASTER_HOST, SPI_CS_IO, SPI_SCK_IO, SPI_MOSI_IO, SPI_MISO_IO, SPI_CLK_SPEED_HZ, devHandle));
}

void eeprom_deinit(spi_device_handle_t devHandle)
{
    ESP_ERROR_CHECK(spi_25LC040_free(SPI_MASTER_HOST, devHandle));
}

void eeprom_write_timestamp(spi_device_handle_t devHandle, uint64_t timestamp)
{
    uint8_t *data = (uint8_t *)(&timestamp);

    ESP_ERROR_CHECK(spi_25LC040_write_page(devHandle, 0, data, 8));

    eeprom_clean_moisture_readings(devHandle);

    eeprom_address = 8;
}

int eeprom_read_timestamp(spi_device_handle_t devHandle, uint64_t *timestamp)
{
    if (eeprom_address <= 8)
        return 0;

    uint8_t data[8];

    for (int i = 0; i < 8; i++)
        ESP_ERROR_CHECK(spi_25LC040_read_byte(devHandle, i, &data[i]));

    *timestamp = *(uint64_t *)data;

    return 1;
}

int eeprom_write_moisture(spi_device_handle_t devHandle, uint8_t moisture)
{
    if (eeprom_address >= 8 && eeprom_address < TOTAL_ADDRESSES)
    {
        ESP_ERROR_CHECK(spi_25LC040_write_byte(devHandle, eeprom_address, moisture));
        eeprom_address++;
        return 1;
    }
    return 0;
}

int eeprom_read_moisture(spi_device_handle_t devHandle, uint16_t address, uint8_t *moisture)
{
    if (address + 8 >= eeprom_address)
        return 0;

    ESP_ERROR_CHECK(spi_25LC040_read_byte(devHandle, address + 8, moisture));
    return 1;
}

int eeprom_read_moisture_timestamp(spi_device_handle_t devHandle, uint16_t address, uint16_t *timestamp)
{
    if (address + 8 >= eeprom_address)
        return 0;

    *timestamp = (address) * ((60 / MEASUREMENTS_PER_HOUR) * 60);
    return 1;
}

int eeprom_read_last_moisture(spi_device_handle_t devHandle, uint8_t *moisture)
{
    return eeprom_read_moisture(devHandle, eeprom_address - 8 - 1, moisture);
}

void eeprom_clean_moisture_readings(spi_device_handle_t devHandle)
{
    uint64_t zeros[2] = {0, 0};
    for (int i = 16; i < 512; i += 16)
    {
        ESP_ERROR_CHECK(spi_25LC040_write_page(devHandle, i, (uint8_t *)zeros, 16));
    }
}