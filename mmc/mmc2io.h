#ifndef _IO

//#include <avr/io.h>
//#include <avr/pgmspace.h>
//#include <avr/eeprom.h>

#define LOG_RESULT

#define RedLED		18
#define GreenLED	19

#define RedLEDOn()		    gpio_put(RedLED,false) 
#define RedLEDOff()		    gpio_put(RedLED,true)
#define RedLedToggle()      gpio_put(RedLED,!gpio_get(RedLED))
#define GreenLEDOn()	    gpio_put(GreenLED,false)
#define GreenLEDOff()	    gpio_put(GreenLED,true)
#define GreenLEDToggle()	gpio_put(GreenLED,!gpio_get(GreenLED))

#define CARD_DETECT	2
#define WRITE_PROT	3

#define CARD_DETECT_MASK	(1<<CARD_DETECT)
#define WRITE_PROT_MASK		(1<<WRITE_PROT)

#define CARDIO_DDR	DDRB
#define CARDIO_PORT	PORTB
#define CARDIO_PIN	PINB

#define CARD_INSERTED()		(~CARDIO_PIN & CARD_DETECT_MASK)
#define CARD_WRITABLE()		(CARDIO_PIN & WRITE_PROT_MASK)

#define ASSERTIRQ()
#define RELEASEIRQ()

#define NOPDelay()		{ asm("nop; nop;"); }

//#define ReadEEPROM(addr)		eeprom_read_byte ((const uint8_t *)(addr))	
//#define WriteEEPROM(addr, val)	eeprom_write_byte ((uint8_t *)(addr), (uint8_t)(val))

#define _IO
#endif
