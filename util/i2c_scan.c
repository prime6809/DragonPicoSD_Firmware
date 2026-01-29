#include <stdio.h>
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "i2c_scan.h"

#define DEBUG_PINS  1

// MostlY borrowed from bus_scan in pico-examples

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_init_bus_scan(i2c_inst_t    *i2c,
                       uint8_t       sda_pin,
                       uint8_t       scl_pin,
                       uint32_t      speed_khz)
{
#if (DEBUG_PINS == 1)
    printf("LCD SDA gpio: %d, SCL gpio: %d, Speed %luKHz\n",sda_pin,scl_pin,speed_khz);
#endif
    // Initialise IIC
    i2c_init(i2c, speed_khz * 1000);

    // Set function of pins to IIC, and enable pullups.
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

    i2c_bus_scan(i2c);
}

void i2c_bus_scan(i2c_inst_t    *i2c)
{
    int     ret;
    uint8_t rxdata;

    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) 
    {
        if (addr % 16 == 0) 
        {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}

