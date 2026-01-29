#ifndef __DMMCTEST_H__
#define __DMMCTEST_H__

#include <stdint.h>

#define DEBUG_STATUS    0

// from DMMC documents.

#define D_IO_Base           0xFF50      // Base of IO registers
#define D_IO_InvMask        0xFFFC      // Mask of address for IO regs, check for valid address
#define D_IO_Mask           0x0003      // Mask of address to select register 

#define D_OS_CMD_REG        0x00        // Command / status register : rw
#define D_OS_LATCH_REG      0x01        // Latch register : rw
#define D_OS_DATA_REG       0x02        // Read data register : rw
#define D_OS_STATUS_REG     0x03        // Status register : ro

#define D_OS_TEST_REG       0x07        // Test register

// New register mapping
#define D_CMD_REG           D_IO_Base+D_OS_CMD_REG       // Command register : rw
#define D_LATCH_REG         D_IO_Base+D_OS_LATCH_REG     // Latch register : rw
#define D_READ_DATA_REG     D_IO_Base+D_OS_DATA_REG      // Read data register : rw
#define D_WRITE_DATA_REG    D_READ_DATA_REG
#define D_STATUS_REG        D_IO_Base+D_OS_STATUS_REG    // Status register : ro

#define D_TEST_REG          D_IO_Base+D_OS_TEST_REG     // Test register w

// D_STATUS_REG bits
#define STATUS_OLDBUSY      0x01        // Old busy signal used on DMMC CPLD/AVR hardware
#define STATUS_AVRR         0x02        // Dragon has written, AVR has read (not used on Pico)
#define STATUS_BUSY         0x80        // We use this then 6809, can busy wait with XXX TST D_STATUS_REG, BMI XXX, note different from AVR! 

// D_RAM_CTRL bits, *NOTE* DIFFERENT FROM ORIGINAL DragonMMC!
// For D_RAM_EANBLE and D_RAM_VEC, 0=ROM on motherboard, 1=RAM in pico.
// Note the first 4 of thse correspond to the output bits in port b of the MCP23017.
// D_RAM_CTRL is now accessed using the commands CMD_GET_RAM_CTRL (0xE8) and 
// CMD_GET_RAM_CTRL (0xE9).

#define D_RAM_ENABLE        0x01        // Enable RAM in $8000-$BFFF, always enabled $C000-$FEFF
#define D_RAM_VEC		    0x02		// Read interrupt vectors from ROM (0) or RAM (1)
#define D_FIRQ_ENABLE       0x04        // Enable routing of Q to CART for auto starting cartridges
#define D_ROM_COCO          0x08        // Sould we access Dragon (0) or CoCo (1) ROM image

#define D_RAM_WP            0x10        // Write protect RAM $8000-$BFFF
#define D_RAM_VEC_WP	    0x20        // Write protect area of RAM containing int vectors.
#define D_NMI_ENABLE        0x40        // Enable generation of NMI on snap button press.

#define D_HOLD_RESET		0x08		// Hold 6809 reset low to wait for Pico initialization to complete 
                                        // NOTE only used by MCP23017

// MCP23017, port b bits. 
// The bits in PORTB_OUT_MASK are simply written from the D_RAM_CTRL register,
// The bits in PORTB_IN_MASK are connected to the switches 
//      Buttons : Cold boot and Snapshot
//      SDCard : Card detect and Write protect   

#define PORTB_MEMCTL_MASK   (D_RAM_ENABLE | D_RAM_VEC | D_FIRQ_ENABLE)
#define PORTB_OUT_MASK      (D_RAM_ENABLE | D_RAM_VEC | D_FIRQ_ENABLE | D_HOLD_RESET)

#define SW_WRITE_PROT       0x10
#define SW_CARD_PRESENT     0x20
#define SW_SNAP             0x40
#define SW_COLD_BOOT        0x80

#define PORTB_IN_MASK       (SW_WRITE_PROT | SW_CARD_PRESENT | SW_SNAP | SW_COLD_BOOT)

// Interrupt pin from MCP23017->Pico
#define PORTB_INT_PIN       15

extern volatile uint8_t Buttons;        // Current button state 

#define IsColdBoot()        ((Buttons & SW_COLD_BOOT)==0)

// Macros for accessing MCP registers
#define MCPWriteReg(reg,val)    MCP23017WriteReg(I2C_PORT,MCPBase,reg,val)
#define MCPReadReg(reg)         MCP23017ReadReg(I2C_PORT,MCPBase,reg)

// Macros for bits in D_RAM_CTRL
#define RAMOveride()        ((D_RAMCtrl & D_RAM_ENABLE)==D_RAM_ENABLE)
#define IsRAMWritable()     ((D_RAMCtrl & D_RAM_WP)==0) 
#define IsIntWritable()     ((D_RAMCtrl & D_RAM_VEC_WP)==0)
#define IntInRAM()          ((D_RAMCtrl & D_RAM_VEC)==D_RAM_VEC)
#define NMIEnabled()        (((D_RAMCtrl & D_NMI_ENABLE)==D_NMI_ENABLE) && ((SAMBits & SAM_TYPE_MASK)==0))

#define MAIN_ROM_BASE   0x8000
#define MAIN_ROM_END    0xBFFF
#define CART_ROM_BASE   0xC000
#define CART_ROM_END    0xFEFF
#define INT_RAM_BASE    0xFFF0

#define BASICROM_SIZE       (1024*16)
#define PAGE_SIZE           (1024*8)
#define CART_SIZE           (PAGE_SIZE*2)
#define VISIBLE_CART_SIZE   CART_SIZE-256   // This is what is visible to the Dragon, as the upper 256 bytes is "hidden" by the I/O area....
#define ALLROM_SIZE         (BASICROM_SIZE+CART_SIZE)
#define VISIBLE_ALLROM_SIZE (BASICROM_SIZE+VISIBLE_CART_SIZE)

// Offsets within ReadRAM and WriteRAM
#define BASIC_ROM_OFFSET    0
#define CART_ROM_OFFSET     BASICROM_SIZE
#define INT_VEC_SIZE        16
#define INT_RAM_OFFSET      (INT_RAM_BASE-MAIN_ROM_BASE)

#define CARTROM_SIZE        (CART_SIZE*2)
#define DRAGON_OFFSET       0x0000      // offset of Dragon rom within MMC rom 
#define COCO_OFFSET         0x4000      // offset of CoCo rom within MMC rom 

#define IO_OFFSET           (D_IO_Base-MAIN_ROM_BASE)

#define D_ROM_BASE          0x8000      // Base of our RAM that is returned when reading, contains both basic mirror and cart code
#define D_ROM_InvMask       0x8000      // Mask to detect valid RAM address
#define D_ROM_Mask          0x7FFF      // Mask for RAM address

extern volatile uint8_t     CartROM[CARTROM_SIZE];              // 6809 client code for Dragon & CoCo, read only

extern volatile uint8_t     ReadRAM[ALLROM_SIZE];               // Reads come from here
extern volatile uint8_t     WriteRAM[ALLROM_SIZE];             // buffer for mirrored/patched Basic ROM + mirrored / loaded CartRAM

extern const uint8_t mmcrom[];

extern volatile uint16_t    SAMBits;

// Write registers.
extern volatile uint8_t     D_CmdReg;       // Command register
extern volatile uint8_t     D_LatchInReg;   // Latch in register
extern volatile uint8_t     D_RAMCtrl;      // Ram mapping control

// We use a 4 byte array for the registers that the 6809 can read, as we can access this
// quicker when the 6809 is reading than checking for individual registers, this points into
// ReadRAM.
extern volatile uint8_t    *ReadRegs;       

extern const uint8_t PIN_E;

#define NMI_PIN             22
#define AssertNMI()         gpio_put(NMI_PIN,false)
#define ClearNMI()          gpio_put(NMI_PIN,true)

#define LED_PIN             25
#define ToggleLED()         gpio_put(LED_PIN, !gpio_get(LED_PIN))
#define LEDOn()             gpio_put(LED_PIN, 1)
#define LEDOff()            gpio_put(LED_PIN, 0)

#define SetBusy()           do { ReadRegs[D_OS_STATUS_REG] |= STATUS_BUSY; LEDOn(); } while(0)
#define ClearBusy()         do { ReadRegs[D_OS_STATUS_REG] &= ~STATUS_BUSY; LEDOff(); } while(0)

#define WriteStatus(st)         do { ReadRegs[D_OS_CMD_REG] = st; } while(0)
#define WriteDataPort(da)       do { globalData[0] = da; globalIndex=0; ReadRegs[D_OS_DATA_REG]=da; } while(0)

#define WriteLatchOut(da)       do { ReadRegs[D_OS_LATCH_REG] = da; } while(0)

// Utility macros
#define SetupIO(gpio,out,up,down)   do {if (gpio < 32) { gpio_init(gpio); gpio_set_dir(gpio,out); gpio_set_pulls(gpio,up,down);}; } while (0)
#define SetupIOH(gpio,out,up,down,hyst) do { SetupIO(gpio,out,up,down); gpio_set_input_hysteresis_enabled(gpio,hyst); } while(0)
#define SetupIOFunction(gpio,out,up,down,func)   do {if (gpio < 32) { gpio_init(gpio); gpio_set_dir(gpio,out); gpio_set_pulls(gpio,up,down); gpio_set_function(gpio, func); }; } while (0)
#define SetupIOFunctionH(gpio,out,up,down,func,hyst) do { SetupIOFunction(gpio,out,up,down,func); gpio_set_input_hysteresis_enabled(gpio,hyst); } while (0)
#define printfc(cond,format,...)    {if (cond) { printf(format,##__VA_ARGS__);}; }
#define IsSet(bits,mask)            ((bits & mask)==mask)
#define BoolStr(b)                  (b ? "true" : "false")

// Serial defines
#define SER_TX      16
#define SER_RX      17 

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
// We will initialize the pins in the main module, so eeprom/MCP23017/RTC don't need to.
#define I2C_PORT    i2c0
#define I2C_SDA     20
#define I2C_SCL     21

#define I2C_SPEED   (400*1000)

// Debug defines, set these to 1 to enable debug of each
#define DEBUG_LATCH 0
#define DEBUG_I2C   1

// 6809 reset vector
#define RESET_VEC 0xFFFE

// Pin to disable DSD being asserted by GAL, when SAM in mode 1
#define NDSD_PIN        14

#endif 

