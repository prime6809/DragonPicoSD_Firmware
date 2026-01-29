/*
    Routines for driving an i2c / iic eeprom.
    Only tested on 24c02x series.
*/

#include "eeprom.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <hardware/gpio.h>

static bool ee_init;

void ee_at_reset(void)
{
    ee_init=false;
}

// Initialize IIC pins ready for eeprom
void init_ee(void)
{
    if(!ee_init)
    {
        // I2C Initialisation. Using it at 400Khz.
        i2c_init(I2C_PORT, 400*1000);
    
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_SDA);
        gpio_pull_up(I2C_SCL);

        ee_init=true;
    }
}

// Read a byte from eeprom
// If return value is -ve an error occured.
int read_ee(uint8_t     iic_addr,       // IIC address
            uint16_t    ee_addr,        // Byte address within the eeprom 
            uint8_t     *toread)        // Pointer to the byte to read into
{
    uint8_t AddrBuff[2];
    int     result;

    // Initialize i2c bus and eeprom, if not already initialized 
    init_ee();

    // Send the read command along with the address to read
    //result = i2c_write_blocking(I2C_PORT,iic_addr,&from,1,true);

    AddrBuff[0]=(ee_addr >> 8) & 0xFF;
    AddrBuff[1]=ee_addr & 0xFF;

#if (EE_ADDRESS_16 == 1)
    result = i2c_write_blocking(I2C_PORT,iic_addr,&AddrBuff[0],2,true);
#else
    result = i2c_write_blocking(I2C_PORT,iic_addr,&AddrBuff[1],1,true);
#endif 

    // if that worked read the data
    if(result >= 0)
    {
        result = i2c_read_blocking(I2C_PORT,iic_addr,toread,1,false);
    }

    return result;
}

// Read a block of bytes from eeprom
// If return value is -ve an error occured.
int read_ee_bytes(uint8_t   iic_addr,       // IIC address
                  uint16_t  ee_addr,        // Byte address within the eeprom 
                  uint8_t   *toread,        // Pointer to the destination buffer
                  uint16_t  bytecount)      // count of bytes to read  
{
    int         result;

    do
    {
        result = read_ee(iic_addr,ee_addr,toread);
        toread++;
        ee_addr++;
        bytecount--;
    } while ((result >= 0) && (bytecount > 0));

    return result;
}

// Write a byte to eeprom
// If return value is -ve an error occured.
int write_ee(uint8_t    iic_addr,       // IIC address
             uint16_t   ee_addr,        // Byte address within the eeprom 
             uint8_t    towrite)        // Byte to be written
{
    uint8_t AddrBuff[3];
    int     result;

    int     retries;

    // Initialize i2c bus and eeprom, if not already initialized 
    init_ee();

    AddrBuff[0]=(ee_addr >> 8) & 0xFF;
    AddrBuff[1]=ee_addr & 0xFF;
    AddrBuff[2]=towrite;

    // initiate the write    
#if (EE_ADDRESS_16 == 1)
    result = i2c_write_blocking(I2C_PORT,iic_addr,&AddrBuff[0],3,false);
#else
    result = i2c_write_blocking(I2C_PORT,iic_addr,&AddrBuff[1],2,false);
#endif 

    // poll the eeprom checking to see if the write has completed
    retries=3;
    do
    {
        sleep_ms(10);
        result = i2c_write_blocking(I2C_PORT,iic_addr,&AddrBuff[1],1,false);
        retries--;
    } while ((result < 0 ) && (retries != 0));

    return result;
}


// Write a block of bytes from eeprom
// If return value is -ve an error occured.
int write_ee_bytes(uint8_t  iic_addr,       // IIC address
                   uint16_t ee_addr,        // Byte address within the eeprom 
                   uint8_t  *towrite,       // Pointer to the source buffer
                   uint16_t bytecount)      // count of bytes to write  
{
    int         result;

    do
    {
        result =     write_ee(iic_addr,ee_addr,*towrite);
        towrite++;
        ee_addr++;
        bytecount--;
    } while ((result >= 0) && (bytecount > 0));

    return result;
}



// Write a string of bytes to eeprom
// If return value is -ve an error occured.
int write_str_ee(uint8_t    iic_addr,       // IIC address
                 uint16_t   ee_addr,        // Byte address withing the eeprom 
                 char       *towrite)       // String to write
{
    uint    Idx;
    int     result = 0;

    for(Idx=0; Idx < strlen(towrite); Idx++)
    {
        result = write_ee(EE_ADDRESS,ee_addr++,towrite[Idx]);
    }

    return result;
}
