/*
    Driver for Microchip MCP23016 16 bit IIC I/O expander.
*/
#include <stdio.h>
#include "hardware/i2c.h"
#include "mcp23017.h"

#define DEBUG_PINS      0
#define DEBUG_SEND      1

#define WRITE_BUF_SIZE  2

void MCP23017Init(i2c_inst_t    *i2c,
                  uint8_t       sda_pin,
                  uint8_t       scl_pin,
                  uint16_t      speed_khz)
{
// IIC init done by main code
#if 0 
    uint baud;
#if (DEBUG_PINS == 1)
    printf("LCD SDA gpio: %d, SCL gpio: %d\n",sda_pin,scl_pin);
#endif
    // Initialise IIC
    baud=i2c_init(i2c, speed_khz * 1000);

printf("Actual baudrate=%d\n",baud);

    // Set function of pins to IIC, and enable pullups.
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
#endif
}

void MCP23017WriteReg(i2c_inst_t *i2c,
                      uint8_t    Address,
                      uint8_t    Register,
                      uint8_t    Data)

{
    uint8_t WriteBuf[WRITE_BUF_SIZE] = {Register, Data};
#if(DEBUG_SEND == 1)
    if (i2c==i2c1)
    printf("MCPWriteReg(%02X,%02X,%02X)\n",Address,Register,Data);
#endif    

    i2c_write_blocking(i2c,Address,WriteBuf,WRITE_BUF_SIZE,false);
}

uint8_t MCP23017ReadReg(i2c_inst_t *i2c,
                        uint8_t    Address,
                        uint8_t    Register)

{
    uint8_t Result;

    i2c_write_blocking(i2c,Address,&Register,1,true);
    i2c_read_blocking(i2c,Address,&Result,1,false);

    return Result;    
}

static uint8_t RegNames[][MCP23017_OLATB] = 
{
    "IODIRA  ",  
    "IODIRB  ",  
    "IOPOLA  ",  
    "IOPOLB  ",  
    "GPINTENA",
    "GPINTENB",
    "DEFVALA ",
    "DEFVALB ",
    "INTCONA ",
    "INTCONB ",
    "IOCONA  ",
    "IOCONB  ",
    "GPPUA   ",
    "GPPUB   ",  
    "INTFA   ",  
    "INTFB   ",  
    "INTCAPA ",
    "INTCAPB ",
    "GPIOA   ",
    "GPIOB   ",
    "OLATA   ",
    "OLATB   "
};

void MCP23017DumpReg(i2c_inst_t *i2c,
                     uint8_t    Address,
                     uint8_t    First,
                     uint8_t    Last)
{
    uint8_t RegVal;

    for(uint8_t Idx=First; Idx<=Last; Idx++)
    {
        RegVal=MCP23017ReadReg(i2c,Address,Idx);

        printf("%s[%02X]=%02X\n",RegNames[Idx],Idx,RegVal);
    }
}