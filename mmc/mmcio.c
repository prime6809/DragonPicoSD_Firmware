#include "mmcio.h"

#include "mmc2io.h"
#include "diskio.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "DragonPicoSD.h"

/* Definitions for MMC/SDC command */
#define CMD0   (0x40+0)    /* GO_IDLE_STATE */
#define CMD1   (0x40+1)    /* SEND_OP_COND (MMC) */
#define ACMD41 (0xC0+41)   /* SEND_OP_COND (SDC) */
#define CMD8   (0x40+8)    /* SEND_IF_COND */
#define CMD16  (0x40+16)   /* SET_BLOCKLEN */
#define CMD17  (0x40+17)   /* READ_SINGLE_BLOCK */
#define CMD24  (0x40+24)   /* WRITE_BLOCK */
#define CMD55  (0x40+55)   /* APP_CMD */
#define CMD58  (0x40+58)   /* READ_OCR */

#define DEBUG_PINS   0

spi_inst_t    *spi;

void InitSPI(void)
{
/*	// SCK, MOSI and SS as outputs, MISO as output
	SPIDDR |= ((1<<SPI_SCK) | (1<<SPI_MOSI) | (1<<SPI_SS));
	SPIDDR &= ~(1<<SPI_MISO);
	
	// initialize SPI interface
	// master mode
	SPCR |= (1<<MSTR);
	
	// select clock phase positive-going in middle of data
	// Data order MSB first
	// switch to f/4 2X = f/2 bitrate
    // SPCR &= ~((1<<CPOL) | (1<<DORD) | (1<<SPR0) | (1<<SPR1));
	
	SPCR &= ~((1<<SPR0) | (1<<SPR1));
	SPSR |= (1<<SPI2X);
	SPCR |= (1<<SPE);
	
	ClearSDCS();
	SPIPORT |= (1<<SPI_SCK);	// set SCK hi*/

#if (DEBUG_PINS == 1)
   uint baud;
   
   // Initialise spi
   baud=spi_init(CARD_SPI, CARD_BAUDRATE);

   printf("MOSI gpio: %d, MISO gpio: %d, SCK gpio, %d, speed %d\n",SPI_MOSI,SPI_MISO,SPI_SCK,CARD_BAUDRATE);
   printf("Actual baudrate=%d\n",baud);
#else 
   spi_init(CARD_SPI, CARD_BAUDRATE);
#endif
    
   //spi_set_format(CARD_SPI,8,SPI_CPOL_1,SPI_CPHA_1,SPI_MSB_FIRST);
   spi_set_format(CARD_SPI,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
   // Set function of pins to SPI, and enable pullups.
   gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
   gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
   gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
   SetupIO(SPI_CS,true,true,false);
   ClearSDCS();
}

BYTE TransferSPI(BYTE output)
{
   uint8_t Result;

	spi_write_read_blocking(CARD_SPI,&output,&Result,1);    // send to SPI
	return Result;						// return received byte
}





/*--------------------------------------------------------------------------

Module Private Functions

---------------------------------------------------------------------------*/

BYTE CardType;


/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void release_spi (void)
{
   ClearSDCS();
   TransferSPI(0xff);
}


/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (
               BYTE cmd,      /* Command byte */
               DWORD arg      /* Argument */
               )
{
   BYTE n, res;

   union
   {
      BYTE b[4];
      DWORD d;
   }
   argbroke;
   argbroke.d = arg;

   if (cmd & 0x80) { /* ACMD<n> is the command sequense of CMD55-CMD<n> */
      cmd &= 0x7F;
      res = send_cmd(CMD55, 0);
      if (res > 1) return res;
   }

   /* Select the card */
   ClearSDCS();
   TransferSPI(0xff);
   AssertSDCS();
   TransferSPI(0xff);

   /* Send a command packet */
   TransferSPI(cmd);           	/* Start + Command index */
   TransferSPI(argbroke.b[3]);  	/* Argument[31..24] */
   TransferSPI(argbroke.b[2]); 	/* Argument[23..16] */
   TransferSPI(argbroke.b[1]); 	/* Argument[15..8] */
   TransferSPI(argbroke.b[0]);  	/* Argument[7..0] */
   n = 0x01;              		/* Dummy CRC + Stop */
   if (cmd == CMD0) n = 0x95;	/* Valid CRC for CMD0(0) */
   if (cmd == CMD8) n = 0x87;	/* Valid CRC for CMD8(0x1AA) */
   TransferSPI(n);

   /* Receive a command response */
   n = 10;                       /* Wait for a valid response in timeout of 10 attempts */
   do {
      res = TransferSPI(0xff);
   } while ((res & 0x80) && --n);

   return res;       /* Return with the response value */
}


/*--------------------------------------------------------------------------

Public Functions

---------------------------------------------------------------------------*/


DSTATUS mmc_status(void)
{
   if (CardType == 0)
      return STA_NODISK;

   // return STA_NOINIT;
   // return STA_NODISK;
   // return STA_PROTECTED;

   return 0;
}


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS mmc_initialize (void)
{
   BYTE n, cmd, ty, ocr[4];
   WORD tmr;

   GreenLEDOn();

   InitSPI();

#if _WRITE_FUNC
   disk_writep(0, 0);     /* Finalize write process if it is in progress */
#endif
   
   ClearSDCS();								/* Make sure card de-selected */
	for (n = 15; n; --n) TransferSPI(0xff);   /* Dummy clocks */

   ty = 0;
   if (send_cmd(CMD0, 0) == 1)
   {
      /* Enter Idle state */
      if (send_cmd(CMD8, 0x1AA) == 1)
      {  /* SDv2 */
         for (n = 0; n < 4; n++)
         {
            ocr[n] = TransferSPI(0xff);      /* Get trailing return value of R7 resp */
         }
         if (ocr[2] == 0x01 && ocr[3] == 0xAA)
         {
            /* The card can work at vdd range of 2.7-3.6V */
            for (tmr = 12000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--) ; /* Wait for leaving idle state (ACMD41 with HCS bit) */

            if (tmr && send_cmd(CMD58, 0) == 0)
            {
               /* Check CCS bit in the OCR */
               for (n = 0; n < 4; n++) ocr[n] = TransferSPI(0xff);
               ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* SDv2 (HC or SC) */
            }
         }
      }
      else
      {                    /* SDv1 or MMCv3 */
         if (send_cmd(ACMD41, 0) <= 1)
         {
            ty = CT_SD1; cmd = ACMD41; /* SDv1 */
         }
         else
         {
            ty = CT_MMC; cmd = CMD1;   /* MMCv3 */
         }
         for (tmr = 25000; tmr && send_cmd(cmd, 0); tmr--) ;   /* Wait for leaving idle state */

         if (!tmr || send_cmd(CMD16, 512) != 0)       /* Set R/W block length to 512 */
         {
            ty = 0;
         }
      }
   }
   CardType = ty;
   release_spi();

   GreenLEDOff();

   return ty ? 0 : STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read sector                                                           */
/*-----------------------------------------------------------------------*/

DRESULT mmc_readsector (BYTE *buff, DWORD lba)
{
   DRESULT res;
   BYTE rc;
   WORD bc;

   GreenLEDOn();

   if (!(CardType & CT_BLOCK)) lba *= 512;      /* Convert to byte address if needed */

   res = RES_ERROR;
   if (send_cmd(CMD17, lba) == 0) {    /* READ_SINGLE_BLOCK */

      bc = 30000;
      do {                    /* Wait for data packet in timeout of 100ms */
         rc = TransferSPI(0xff);
      } while (rc == 0xFF && --bc);

      if (rc == 0xFE)
      {
         /* A data packet arrived */
         for (bc = 0; bc < 512; ++bc)
         {
            buff[bc] = TransferSPI(0xff);
         }
         /* Skip CRC */
         TransferSPI(0xff);
         TransferSPI(0xff);

         res = RES_OK;
      }
   }

   release_spi();

   GreenLEDOff();

   return res;
}



DRESULT mmc_readsector_halp(BYTE *buff, DWORD lba, BYTE top)
{
   DRESULT res;
   BYTE rc;
   WORD bc;

   GreenLEDOn();

   if (!(CardType & CT_BLOCK)) lba *= 512;      /* Convert to byte address if needed */

   res = RES_ERROR;
   if (send_cmd(CMD17, lba) == 0) {    /* READ_SINGLE_BLOCK */

      bc = 30000;
      do {                    /* Wait for data packet in timeout of 100ms */
         rc = TransferSPI(0xff);
      } while (rc == 0xFF && --bc);

      if (rc == 0xFE)
      {
         if (top)
         {
            for (bc = 0; bc < 256; ++bc)
            {
               TransferSPI(0xff);
            }
         }

         /* A data packet arrived */
         for (bc = 0; bc < 256; ++bc)
         {
            buff[bc] = TransferSPI(0xff);
         }

         if (!top)
         {
            for (bc = 0; bc < 256; ++bc)
            {
               TransferSPI(0xff);
            }
         }

         /* Skip CRC */
         TransferSPI(0xff);
         TransferSPI(0xff);

         res = RES_OK;
      }
   }

   release_spi();

   GreenLEDOff();

   return res;
}


/*-----------------------------------------------------------------------*/
/* Write sector                                                  */
/*-----------------------------------------------------------------------*/

DRESULT mmc_writesector(BYTE *buff, DWORD sa)
{
   DRESULT res;
   DWORD bc;

   GreenLEDOn();

   res = RES_ERROR;

   if (!(CardType & CT_BLOCK)) sa *= 512; /* Convert to byte address if needed */

   if (send_cmd(CMD24, sa) == 0)
   {
      TransferSPI(0xFF);
      TransferSPI(0xFE);

      for (bc = 0; bc < 512; ++bc)
      {
         TransferSPI(buff[bc]);
      }

      TransferSPI(0);
      TransferSPI(0);

      if ((TransferSPI(0xff) & 0x1F) == 0x05)
      {
         /* Receive data resp and wait for end of write process in timeout of 300ms */
         for (bc = 128000; TransferSPI(0xff) != 0xFF && bc; bc--) ;   /* Wait ready */
         if (bc) res = RES_OK;

         release_spi();
      }
   }

   GreenLEDOff();

   return res;
}
