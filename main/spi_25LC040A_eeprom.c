#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "spi_25LC040A_eeprom.h"

#define READ 0x03
#define WRITE 0x02
#define WRDI 0x04
#define WREN 0x06
#define RDSR 0x05
#define WRSR 0x01

#define WIP_MASK 0x01
#define WEL_MASK 0x02
#define BP_MASK 0x0C

esp_err_t spi_25LC040_init(spi_host_device_t masterHostId, int csPin, int sckPin, int mosiPin, int misoPin,
                           int clkSpeedHz, spi_device_handle_t *pDevHandle)
{
    esp_err_t ret;

    spi_bus_config_t spiBusConfig = {
        .mosi_io_num = mosiPin,
        .miso_io_num = misoPin,
        .sclk_io_num = sckPin,
        .max_transfer_sz = 18,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    spi_device_interface_config_t spiDevConfig = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = clkSpeedHz,
        .spics_io_num = csPin,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 1,
    };

    ret = spi_bus_initialize(masterHostId, &spiBusConfig, SPI_DMA_DISABLED);
    if (ret != ESP_OK)
        return ret;

    ret = spi_bus_add_device(masterHostId, &spiDevConfig, pDevHandle);
    if (ret != ESP_OK)
        spi_bus_free(masterHostId);

    return ret;
}

esp_err_t spi_25LC040_free(spi_host_device_t masterHostId, spi_device_handle_t devHandle)
{
    esp_err_t ret;

    ret = spi_bus_remove_device(devHandle);

    if (ret == ESP_OK)
        ret = spi_bus_free(masterHostId);

    return ret;
}

esp_err_t spi_25LC040_read_byte(spi_device_handle_t devHandle,
                                uint16_t address, uint8_t *pData)
{
    uint8_t status;
    do
    {
        spi_25LC040_read_status(devHandle, &status);
    } while (status & WIP_MASK);

    uint8_t instruction = READ | ((address & 0x0100) >> 5);
    uint8_t txData[2] = {instruction, address};

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = 2 * 8;
    spiTrans.tx_buffer = txData;
    spiTrans.rxlength = 8;
    spiTrans.rx_buffer = pData;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}

esp_err_t spi_25LC040_write_byte(spi_device_handle_t devHandle,
                                 uint16_t address, uint8_t data)
{
    uint8_t status;
    do
    {
        spi_25LC040_read_status(devHandle, &status);
    } while (status & WIP_MASK);

    spi_25LC040_write_enable(devHandle);
    vTaskDelay(10);

    uint8_t instruction = WRITE | ((address & 0x0100) >> 5);

    uint8_t txData[3] = {instruction, address, data};

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = 3 * 8;
    spiTrans.tx_buffer = txData;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}

esp_err_t spi_25LC040_write_page(spi_device_handle_t devHandle,
                                 uint16_t address, const uint8_t *pBuffer, uint8_t size)
{
    uint8_t status;
    do
    {
        spi_25LC040_read_status(devHandle, &status);
    } while (status & WIP_MASK);

    uint8_t maxSize = 16 - (address % 16);

    if (size > maxSize)
        return ESP_ERR_INVALID_SIZE;

    spi_25LC040_write_enable(devHandle);
    vTaskDelay(10);

    uint8_t instruction = WRITE | ((address & 0x0100) >> 5);

    uint8_t txData[2 + size];
    txData[0] = instruction;
    txData[1] = address;
    for (int i = 2; i < size + 2; i++)
        txData[i] = pBuffer[i - 2];

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = (2 + size) * 8;
    spiTrans.tx_buffer = txData;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}

esp_err_t spi_25LC040_write_enable(spi_device_handle_t devHandle)
{
    uint8_t txData[1] = {WREN};

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = 8;
    spiTrans.tx_buffer = txData;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}

esp_err_t spi_25LC040_write_disable(spi_device_handle_t devHandle)
{
    uint8_t txData[1] = {WRDI};

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = 8;
    spiTrans.tx_buffer = txData;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}

esp_err_t spi_25LC040_read_status(spi_device_handle_t devHandle, uint8_t *pStatus)
{
    uint8_t txData[1] = {RDSR};

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = 8;
    spiTrans.tx_buffer = txData;
    spiTrans.rxlength = 8;
    spiTrans.rx_buffer = pStatus;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}

esp_err_t spi_25LC040_write_status(spi_device_handle_t devHandle, uint8_t status)
{
    uint8_t txData[2] = {WRSR, status};

    spi_transaction_t spiTrans;
    memset(&spiTrans, 0, sizeof(spiTrans));

    spiTrans.length = 2 * 8;
    spiTrans.tx_buffer = txData;

    return spi_device_polling_transmit(devHandle, &spiTrans);
}
