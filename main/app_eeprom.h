#include "spi_25LC040A_eeprom.h"

#define SPI_MASTER_HOST SPI3_HOST
#define SPI_CS_IO 16
#define SPI_SCK_IO 17
#define SPI_MOSI_IO 5
#define SPI_MISO_IO 18
#define SPI_CLK_SPEED_HZ 1000000

#define MEASUREMENTS_PER_HOUR 20 //  every 3 minutes
#define MEASUREMENTS_PER_DAY (MEASUREMENTS_PER_HOUR * 24)
#define TOTAL_ADDRESSES (MEASUREMENTS_PER_DAY + 8)

void eeprom_init(spi_device_handle_t *devHandle);
void eeprom_deinit(spi_device_handle_t devHandle);
void eeprom_write_timestamp(spi_device_handle_t devHandle, uint64_t timestamp);
int eeprom_read_timestamp(spi_device_handle_t devHandle, uint64_t *timestamp);
int eeprom_write_moisture(spi_device_handle_t devHandle, uint8_t moisture);
int eeprom_read_moisture(spi_device_handle_t devHandle, uint16_t address, uint8_t *moisture);
int eeprom_read_moisture_timestamp(spi_device_handle_t devHandle, uint16_t address, uint16_t *timestamp);
int eeprom_read_last_moisture(spi_device_handle_t devHandle, uint8_t *moisture);
void eeprom_clean_moisture_readings(spi_device_handle_t devHandle);