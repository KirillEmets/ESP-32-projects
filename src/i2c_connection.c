#include "i2c_connection.h"

#define I2C_TIMEOUT_MS 100

static i2c_master_bus_handle_t s_bus;

esp_err_t i2c_connection_init(gpio_num_t sda, gpio_num_t scl) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&bus_config, &s_bus);
}

esp_err_t i2c_connection_add_device(uint8_t address, uint32_t speed_hz, i2c_device_t *out_device) {
    if (s_bus == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = speed_hz,
    };

    return i2c_master_bus_add_device(s_bus, &dev_config, out_device);
}

esp_err_t i2c_connection_write(i2c_device_t device, const uint8_t *data, size_t len) {
    return i2c_master_transmit(device, data, len, I2C_TIMEOUT_MS);
}

esp_err_t i2c_connection_write_reg(i2c_device_t device, uint8_t reg, const uint8_t *data, size_t len) {
    // Send register and payload back to back without copying into a temp buffer
    i2c_master_transmit_multi_buffer_info_t buffers[] = {
        {.write_buffer = &reg, .buffer_size = 1},
        {.write_buffer = data, .buffer_size = len},
    };

    return i2c_master_multi_buffer_transmit(device, buffers, len ? 2 : 1, I2C_TIMEOUT_MS);
}

esp_err_t i2c_connection_read_reg(i2c_device_t device, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(device, &reg, 1, data, len, I2C_TIMEOUT_MS);
}
