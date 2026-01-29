#ifndef _MMC

#include "ff.h"

/* SPI stuff : PHS 2010-06-08 */
#define SPIPORT			PORTB
#define SPIPIN			PINB
#define	SPIDDR			DDRB

#define SPI_CS			13
#define SPI_MOSI		11
#define SPI_MISO		12
#define	SPI_SCK			10

#define CARD_SPI        spi1
#define CARD_BAUDRATE   (48 * 1000 * 1000)

#define AssertSDCS()	{ gpio_put(SPI_CS,false); };
#define ClearSDCS()		{ gpio_put(SPI_CS,true); };

#define MMC_SEL()       (SPI_CS_PIN & SPI_SS_MASK)==0

// In the Pi this needs moving to IIC
// For PCF2129AT RTC, in SPI mode, the RTC shares MOSI, MISO and SCK with the
// SD/MMC, so we only need to define a chip select for the RTC

/* Card type flags (CardType) */
#define CT_MMC 			0x01 				/* MMC ver 3 */
#define CT_SD1 			0x02 				/* SD ver 1 */
#define CT_SD2 			0x04 				/* SD ver 2 */
#define CT_SDC 			(CT_SD1|CT_SD2) 	/* SD */
#define CT_BLOCK 		0x08 				/* Block addressing */

void InitSPI(void);
BYTE TransferSPI(BYTE output);

#define _MMC
#endif

