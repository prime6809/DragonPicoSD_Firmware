#ifndef __I2C_SCAN_H__
#define __I2C_SCAN_H__

#include "hardware/i2c.h"

void i2c_init_bus_scan(i2c_inst_t    *i2c,
                       uint8_t       sda_pin,
                       uint8_t       scl_pin,
                       uint32_t      speed_khz);

void i2c_bus_scan(i2c_inst_t    *i2c);

#endif
