#include "spi_connection.h"

#define SPI_HOST_ID   SPI2_HOST
#define SPI_READ_BIT  0x80

static bool s_bus_ready;

esp_err_t spi_connection_init(gpio_num_t sclk, gpio_num_t mosi, gpio_num_t miso) {
    spi_bus_config_t bus_config = {
        .sclk_io_num = sclk,
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    esp_err_t err = spi_bus_initialize(SPI_HOST_ID, &bus_config, SPI_DMA_CH_AUTO);
    s_bus_ready = (err == ESP_OK);
    return err;
}

esp_err_t spi_connection_add_device(gpio_num_t cs, uint32_t speed_hz, spi_device_t *out_device) {
    if (!s_bus_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    // Register byte goes out in the address phase, payload in the data phase
    spi_device_interface_config_t dev_config = {
        .address_bits = 8,
        .mode = 0,
        .clock_speed_hz = speed_hz,
        .spics_io_num = cs,
        .queue_size = 1,
    };

    return spi_bus_add_device(SPI_HOST_ID, &dev_config, out_device);
}

esp_err_t spi_connection_write_reg(spi_device_t device, uint8_t reg, const uint8_t *data, size_t len) {
    spi_transaction_t t = {
        .addr = reg & ~SPI_READ_BIT,
        .length = len * 8,
        .tx_buffer = data,
    };

    return spi_device_polling_transmit(device, &t);
}

esp_err_t spi_connection_read_reg(spi_device_t device, uint8_t reg, uint8_t *data, size_t len) {
    spi_transaction_t t = {
        .addr = reg | SPI_READ_BIT,
        .length = len * 8,
        .rx_buffer = data,
    };

    return spi_device_polling_transmit(device, &t);
}
