/*
    Driver for Microchip MCP23017 16 bit IIC I/O expander.
*/

#ifndef __MCP23017_H__
#define __MCP23017_H__

#include "hardware/i2c.h"

// MCP I2C addresses are 0x20-0x27
#define MCPBase         0x20

// MCP23017 registers, note assumes IOCON.BANK=0

#define MCP23017_IODIRA     0x00
#define MCP23017_IODIRB     0x01
#define MCP23017_IOPOLA     0x02
#define MCP23017_IOPOLB     0x03
#define MCP23017_GPINTENA   0x04
#define MCP23017_GPINTENB   0x05
#define MCP23017_DEFVALA    0x06
#define MCP23017_DEFVALB    0x07
#define MCP23017_INTCONA    0x08
#define MCP23017_INTCONB    0x09
#define MCP23017_IOCONA     0x0A
#define MCP23017_IOCONB     0x0B
#define MCP23017_IOCON      MCP23017_IOCONA
#define MCP23017_GPPUA      0x0C
#define MCP23017_GPPUB      0x0D
#define MCP23017_INTFA      0x0E
#define MCP23017_INTFB      0x0F
#define MCP23017_INTCAPA    0x10
#define MCP23017_INTCAPB    0x11
#define MCP23017_GPIOA      0x12
#define MCP23017_GPIOB      0x13
#define MCP23017_OLATA      0x14
#define MCP23017_OLATB      0x15

// IOCON register bits
#define MCP23017_IOCON_BANK     0x80
#define MCP23017_IOCON_MIRROR   0X40
#define MCP23017_IOCON_SEQOP    0X20
#define MCP23017_IOCON_DISSLW   0X10
#define MCP23017_IOCON_HAEN     0X08    // MCP23s17 only.
#define MCP23017_IOCON_ODR      0X04
#define MCP23017_IOCON_INTPOL   0X02

// Note we currently use 8 bit register mode

void MCP23017Init(i2c_inst_t    *i2c,
                  uint8_t       sda_pin,
                  uint8_t       scl_pin,
                  uint16_t      speed_khz);

void MCP23017WriteReg(i2c_inst_t *i2c,
                      uint8_t    Address,
                      uint8_t    Register,
                      uint8_t    Data);

uint8_t MCP23017ReadReg(i2c_inst_t *i2c,
                        uint8_t    Address,
                        uint8_t    Register);

void MCP23017DumpReg(i2c_inst_t *i2c,
                     uint8_t    Address,
                     uint8_t    First,
                     uint8_t    Last);

#endif