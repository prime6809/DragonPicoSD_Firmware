/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pico/stdio.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/multicore.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "DragonPicoSD.h"
#include "writebus_6809.pio.h"
#include "readbus_6809.pio.h"
#include "mmc/ff.h"
#include "mmc/mmc2def.h"
#include "mmc/mmc2.h"
#include "mmc/mmc2io.h"
#include "hw/eeprom.h"
#include "hw/mcp23017.h"
#include "hw/pcf2129rtc.h"
#include "hexout.h"
#include "sam.h"

#if DEBUG_I2C
#include "i2c_scan.h"
#endif

#if DEVEL
#define NOW "PiPico2-devel: compiled at " __TIME__ " on " __DATE__ 
#else
#define NOW "PiPico2: compiled at " __TIME__ " on " __DATE__ 
#endif

// Pins used by the input and output PIO state machines.
const uint8_t SEL1_PIN = writebus_PIN_SEL1;
const uint8_t SEL2_PIN = writebus_PIN_SEL1 + 1;
const uint8_t SEL3_PIN = writebus_PIN_SEL1 + 2;
const uint8_t PIN_E    = writebus_PIN_E;
const uint8_t PIN_RW   = writebus_PIN_RW;

static PIO pio = pio1;

volatile unsigned char 	globalData[GLOBUFFSIZE];		// Global buffer used for data exchange with Dragon
volatile unsigned char	globalPlatform;					// Global platform byte.
volatile BYTE			globalStreaming;				// Global streaming, if true we are streaming rather 
														// than using command response driven protocol.
volatile BYTE	        globalWriteFlag;				// Used whilst streaming to flag we need to write data to disk.
volatile WORD	        globalWriteBase;				// Base offset of streamed data to be written.
volatile BYTE	        dataValue;						// data read whilst streaming.
BYTE 					configByte;						// System configuration byte
BYTE 					blVersion;						// boot loder version
BYTE					CardInserted;					// Flag to indicate card is inserted
BYTE					CardInsertedNow;				// last value of above flag.
const char __in_flash() CompileTime[]  = NOW;		    // Time firmware compiled.

volatile bool       reset_flag  = false;

// We have two equally sized (32K) RAM buffers, all 6809 Reads come from ReadWRAM, and all 6809 wites go to WriteRAM
// The CMD_SET_RAM_CTRL command handles simulating paging by using memcpy to copy from WriteRAM to ReadRAM when needed.
// An upshot of this is if you write to WriteRAM, it will not be available to ReadRAM until you do a CMD_SET_RAM_CTRL,
// An easy way to do this is to do a CMD_GET_RAM_CTRL to get the RAM_CTRL, and then just write it back with CMD_SET_RAM_CTRL.
volatile uint8_t    ReadRAM[ALLROM_SIZE];               // Reads come from here
volatile uint8_t    WriteRAM[ALLROM_SIZE];              // buffer for mirrored/patched Basic ROM + mirrored / loaded CartRAM

// Saved SAM bits
volatile uint16_t   SAMBits;
volatile uint16_t   OldSAMBits;

// Registers used to communicate with 6809.
// These are the *WRITE* registers, for performance reasons the read registers are stored in
// ReadRAM, with ReadRegs pointing to their base.
volatile uint8_t    D_CmdReg;       // Command register
volatile uint8_t    D_LatchInReg;   // Latch in register
volatile uint8_t    D_RAMCtrl;      // Ram mapping control 

volatile uint8_t    *ReadRegs;       

volatile uint32_t   StartTime;
volatile uint32_t   EndTime;

volatile uint8_t    Buttons;        // Current button state

// Used by memory interface on core0 to flag writes to command and latch registers.
// Only SET by core0 and CLEARD by core1
volatile bool       CmdSet  = false; 
volatile bool       Latched = false;

volatile uint8_t    TestReg;
volatile bool       TestW = false;

static uint16_t     last = 0;       // Last address accessed
volatile uint16_t   samlast = 0;    // Address that SAM write came from 

// Firmware state
enum FWState {
    NotReady,                       // Just after power on, code initializing, 6809 held in reset
    Ready,                          // Code has initialized, and is ready to serve requests, 6809 may be released
    Running                         // 6809 is running, our code is serving requests.
};

volatile enum FWState IsReady = NotReady;

// Get the 6809 code.
#include "picommcrom.c"

void poll_switches();

static void InitSerial()
{
    // Init uart pins
    gpio_set_function(SER_TX, GPIO_FUNC_UART);
    gpio_set_function(SER_RX, GPIO_FUNC_UART);
}

static void InitialiseIO()                
{
    printf("InitialiseIO()\n");

    // pins 2 to 9 are used to read the 6502 / 6809 bus - 8 bits at a time
    for (uint pin = writebus_PIN_A0; pin <= (writebus_PIN_A0+8); pin++)
    {
        SetupIOFunctionH(pin,false,false,false,GPIO_FUNC_PIO1,false);
    }

    // Output enable for the 74lvc245 buffers
    SetupIOFunction(SEL1_PIN,true,true,false,GPIO_FUNC_PIO1);
    SetupIOFunction(SEL2_PIN,true,true,false,GPIO_FUNC_PIO1);
    SetupIOFunction(SEL3_PIN,true,true,false,GPIO_FUNC_PIO1);

    // Input pins for E,Q & R/W (Q not currently used)
    SetupIOH(PIN_E,false,false,true,false);
    SetupIOH(PIN_RW,false,true,false,false);
    //SetupIOH(PIN_Q, false, false, false, false);

    // LED (on pico)
    SetupIO(LED_PIN,true,false,false);

    //IIC lines
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, I2C_SPEED);

    //gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    //gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    SetupIOH(I2C_SDA,true,true,false,true);
    SetupIOH(I2C_SCL,true,true,false,true);

    read_ee(EE_ADDRESS,EE_SYSFLAGS+PLAT_SYSTEM,&configByte);
    
    // Only for debug
#if DEBUG_I2C
    i2c_bus_scan(I2C_PORT);
#endif

    // SD Card LEDS.
    SetupIO(RedLED,true,false,false);
    SetupIO(GreenLED,true,false,false);

    // Setup Port B output register outputs.
    // Note for PCF 0=output, 1=input
    // Note 2, we write the Data register *FIRST* to make sure valid data is output.
    MCPWriteReg(MCP23017_GPIOB,D_HOLD_RESET);
    MCPWriteReg(MCP23017_IODIRB,PORTB_IN_MASK); 

    // Configure inputs
    MCPWriteReg(MCP23017_GPINTENB,PORTB_IN_MASK);       // Switches cause interrupt
    MCPWriteReg(MCP23017_DEFVALB,PORTB_IN_MASK);        // default value high
    MCPWriteReg(MCP23017_INTCONB,0);                    // compare against current value
    MCPWriteReg(MCP23017_GPPUB,PORTB_IN_MASK);          // Enable pullups
    MCPWriteReg(MCP23017_IOCON,MCP23017_IOCON_INTPOL);  // Set int to active high

    // Setup interrupt input pin for MCP23017.
    SetupIO(PORTB_INT_PIN,false,false,false);

    // Setup NMI output pin.
    gpio_put(NMI_PIN,true);
    SetupIO(NMI_PIN,true,false,false);
    gpio_put(NMI_PIN,true);
    
    // Get initial value of switches
    poll_switches();

    // Setup no DSD pin to signal no-overide to external logic
    SetupIO(NDSD_PIN,true,false,false);
    gpio_put(NDSD_PIN,false);
}

// Toggle the leds so that the user knows the system is ready.
void flag_init(void)
{
	GreenLEDOn();
	busy_wait_ms(100);
	RedLEDOn();
	busy_wait_ms(100);
	
	GreenLEDOff();
	busy_wait_ms(100);
	RedLEDOff();
}

void InitVars(void)

{
    printf("InitVars()\n");

    reset_flag  = false;
    ReadRegs=&ReadRAM[IO_OFFSET];
    ReadRegs[D_OS_STATUS_REG]=0;

    memset((void *)ReadRAM,1,ALLROM_SIZE);
    memset((void *)WriteRAM,1,ALLROM_SIZE);

    // Initially copy the Dragon version of the 6809 code to the ReadRAM, during boot the 6809 code will
    // detect the machine type and set the D_ROM_COCO bit in the ram control if needed
    memcpy((uint32_t*)&ReadRAM[CART_ROM_OFFSET],(uint32_t*)&mmcrom[DRAGON_OFFSET],VISIBLE_CART_SIZE);

    ResetLatch(BytesLatch);
    memset((uint8_t *)&BytesLatch,0,sizeof(BytesLatch));
    
    D_RAMCtrl=0;
//    memset((uint8_t *)&IntRAM[0],0,INT_VEC_SIZE);
}


#if DEBUG_LATCH
void DumpLatch(BytesLatch_t *Latch)
{
	printf("Index=%d, Bytes=",Latch->Index);

	for(uint8_t Idx=0; Idx<BYTESLATCHSIZE;Idx++)
		printf("%02X ",Latch->Bytes[Idx]);

	printf("\n\n");
}
#endif 

// Assert or clear 6809 reset line via MCP23017.
static inline void Reset6809(bool state)
{
    uint8_t Reset = MCPReadReg(MCP23017_GPIOB);
    
    if (state)
        Reset |= D_HOLD_RESET;
    else 
        Reset &= ~D_HOLD_RESET;
    
    MCPWriteReg(MCP23017_GPIOB,Reset);
}

void core1_func();

static semaphore_t core1_initted;

bool updated;

#define CSI "\x1b["

void __no_inline_not_in_flash_func(main_loop())
{

    printf("Core 0 main loop\n");

    IsReady = Ready;

    while (true)
    {
        // Get event from SM 0
        uint32_t reg = pio_sm_get_blocking(pio, 0);

        // Get the address
        uint16_t address = reg & 0xFFFF;
 
        // Is it a read or write opertaion?
        // First handle read
        if (!(reg & 0x1000000))
        {
            // Read timing is Critical, especially in 1.77MHz mode!
            // We have to determine the data byte and return it  to the read 
            // state machine, *BEFORE* the falling edge of the E clock.
            // To do this we access one big block of memory containing both our ROM
            // image, the mirrored (if enabled) Basic ROM and the Output registers.
            /// There simply isn't enough time to do this any other way.
            //
            // **Note** that wilst the data is always sent by the pico, external logic 
            // controls if that data is gated onto the 6809 address bus which is why 
            // we don't need to worry about it here.
//            if (D_ROM_BASE == (address & D_ROM_BASE))
            {
                // Send data byte to the state machine.
                pio_sm_put(pio, 1, 0xFF00 | ReadRAM[address & D_ROM_Mask]);

                // If we just read the READ_DATA register, load it with the next byte from
                // the buffer, note this happens after the data is sent to the SM so is not as
                // timing critical as it has the E low period to complete.
                if (D_READ_DATA_REG == address)
                {
                    if (globalIndex < GLOBUFFSIZE)
					    ReadRegs[D_OS_DATA_REG] = globalData[++globalIndex];
                }
            }
            // Check for reset vector fetch, if so flag reset
            // 2024-11-07, this may not be a reset though, considder ldx $FFFE which will read
            // the reset vector without being a reset!
            if ((RESET_VEC+1 == address) && (RESET_VEC == last))
            {
                reset_flag = true;
            }   
        }
        else    // now handle write
        {
            // Get the data
            uint8_t data = (reg & 0xFF0000) >> 16;

            // Write data processing is not as time critical because nothing needs to be returned to
            // the 6809 side, so we have all of the E low period to complete this.

            // Update SAM bits when written to
            if ((address >= SAM_BASE) && (address <= SAM_END))
            {
                uint8_t     sam_data = GetSAMData(address);
                uint16_t    sam_mask = GetSAMDataMask(address);

                if (sam_data)
                {
                    SAMBits |= sam_mask;
                }
                else
                {
                    SAMBits &= ~sam_mask;
                }

                // Flag to the external logic which SAM map mode we are in so that it only
                // overrides the internal address decoding in map type 0.
                gpio_put(NDSD_PIN,IsSet(SAMBits,SAM_TYPE_MASK));

                // before last updated!
                samlast = last;
            }
            else if (D_CMD_REG == address)
            {
                SetBusy();
                D_CmdReg=data;
                CmdSet=true;
            }
            else if (D_LATCH_REG == address)
            {
                SetBusy();
                D_LatchInReg=data;
                Latched=true;
            }
            else if(D_WRITE_DATA_REG == address)
            {            	
                globalData[globalIndex] = data;
				if (globalIndex < GLOBUFFSIZE)
					++globalIndex;

            }
            else if(D_TEST_REG == address)
            {
                TestReg=data;
                TestW=true;
                WriteRAM[address & D_ROM_Mask]=~data;
            }
            else if(D_ROM_BASE == (address & D_ROM_BASE))
            {
                WriteRAM[address & D_ROM_Mask]=data;
            }
        }
        
        if (0xFFFF != address)
            last = address; 
    }
}

// Decode SAM bits
void ShowSAMBits(uint16_t   Bits)
{
    uint8_t Temp = GetSAMRAMSize(Bits);

    if (DEBUG_ENABLED)
    {
        printf("SAMBits:%04X, ",Bits);
        printf("Map:%d, ",GetSAMMapType(Bits));
        printf("Mem size:");

        switch (Temp)
        {
            case 0 : printf("4K"); break;
            case 1 : printf("16K"); break;
            case 2 : printf("64K"); break;
            case 3 : printf("64K static"); break;
        }

        Temp=GetSAMCPUSpeed(Bits);
        printf(", CPUSpeed:");

        switch(Temp)
        {
            case 0 : printf("Slow"); break;
            case 1 : printf("Address dependent"); break;
            default : printf("Fast"); break;
        }

        printf(", Page:%d, ",GetSAMPage(Bits));
        printf("Vaddr: %04X, ",GetVidMemBase(Bits));
        printf("VMdoe: %d\n",GetSAMMode(Bits));
    }
}

static inline void ShowSAM(void)
{
    if(DEBUG_ENABLED)
    {
        printf("SAM bits updated old=%04X, new=%04X, last=%04X\n",OldSAMBits,SAMBits,samlast);
        ShowSAMBits(SAMBits);
    }
    OldSAMBits=SAMBits;
}

int main(void)
{
    // Note this is a 100MHz overclock, but the pi-pico2 seems to cope with this OK, as determined by 
    // many people using the pi-pico and pi-pico2.
    uint sys_freq = 250000; 
    bool ClockSet;

//    __breakpoint();

    if (sys_freq > 250000)
    {
        vreg_set_voltage(VREG_VOLTAGE_1_25);
    }

    ClockSet=set_sys_clock_khz(sys_freq, true);

    InitSerial();
    stdio_init_all();

#if PICO_RP2350
    printf("I'm an RP2350 ");
#ifdef __riscv
    printf("running RISC-V\n");
#else
    printf("running ARM\n");
#endif
#else
    printf("I'm an RP2040\n");
#endif

    if (ClockSet)
        printf("\nClock set to %dkhz\n",sys_freq);
    else
        printf("\nCould **NOT** set clock set to %dkhz\n",sys_freq);


    InitialiseIO();
    //configByte=0xff;
    InitVars();

    // create a semaphore to be posted when core 1 init is complete
    sem_init(&core1_initted, 0, 1);

    // launch main program on core 1
    multicore_launch_core1(core1_func);

    // wait for initialization of core1 to be complete
    sem_acquire_blocking(&core1_initted);
 
    uint offset = pio_add_program(pio, &writebus_program);
    writebus_program_init(pio, 0, offset);
    pio_sm_set_enabled(pio, 0, true);

    offset = pio_add_program(pio, &readbus_program);
    readbus_program_init(pio, 1, offset);
    pio_sm_set_enabled(pio, 1, true);

    printf("starting main loop\n");
    main_loop();
}

extern void SetRAMCtrl();

void check_reset(void)
{
    if (reset_flag)
    {
        ReadRegs[D_OS_STATUS_REG]=0;
        reset_flag  = false;

        SAMBits=0;
        OldSAMBits=SAMBits;

        if(IsColdBoot())
        {
            D_RAMCtrl=0;
            SetRAMCtrl();
            // Reset cart RAM
            memcpy((uint32_t*)&ReadRAM[CART_ROM_OFFSET],(uint32_t*)&mmcrom[DRAGON_OFFSET],VISIBLE_CART_SIZE);
            
            printfc(DEBUG_ENABLED,"Cold");
        }
        else
        {
            printfc(DEBUG_ENABLED,"Warm");
        }

        printfc(DEBUG_ENABLED," Reset!\n");
        ResetLatch(BytesLatch);
    }
}

// Check for changed buttons & switches from the mcp23017.
void poll_switches()
{
    uint8_t IntSrc;     // Inturrupt source
    uint8_t NewButtons; // New button states

    IntSrc=MCPReadReg(MCP23017_INTFB);      // Get interrupt flag register
    NewButtons=MCPReadReg(MCP23017_GPIOB);  // get the button state

    if(IntSrc & SW_WRITE_PROT)
    {
        printfc(DEBUG_ENABLED,"Write protect\n");
    }

    if(IntSrc & SW_CARD_PRESENT)
    {
        printfc(DEBUG_ENABLED,"Card detect\n");
    }

    // Generate an NMI interrupt to the 6809, if the snapot button is
    // pressed and interrupts are enabled.
    if(IntSrc & SW_SNAP)
    {
        printfc(DEBUG_ENABLED,"Snapshot\n");

        // Only trigger if enabled and button is down....
        if (NMIEnabled() && ((NewButtons & SW_SNAP)==0))
        {
            AssertNMI();
            busy_wait_us(10);
            ClearNMI();
        }
    }

    if(IntSrc & SW_COLD_BOOT)
    {
        printfc(DEBUG_ENABLED,"Cold boot\n");
    }

    // Handle card insertion/removal.
    if(IsSet(IntSrc,SW_CARD_PRESENT))
    {
        if(IsSet(NewButtons,SW_CARD_PRESENT))
            DismountCard();
        else
            InitNewCard(); 
    }

    Buttons=NewButtons & PORTB_IN_MASK;

    printfc(DEBUG_ENABLED,"Buttons=%02X\n",Buttons);
}

void core1_func()
{
    // core 1 is used for all the main function handling of the system, so that
    // core 0 may be dedicated to the 6809 bus interface which is timing critical.
    // When a command(or latched byte) comes in from the 6809, core 0 sets the busy flag
    // (so the 6809 knows to wait), and then sets one of CmdSet or Latched.
    // core 1 as part of it's main loop below, checks for one of these flags and then 
    // dispatches it. When done it clears CmdSet/Latched (as needed) and busy, so that
    // the 6809 know the command is done and can get status/results.
    sem_release(&core1_initted);

    printf("core 1 init\n");

	printf("Init RTC\n");
    rtcInit();

    printf("RTC Date/Time: %s\n",rtcDateTimeASCII());

    // Initialise main code
    at_initprocessor();
    flag_init();
    
    printf("Core 1 main loop\n");

    while (true)
    {   
        // On power on the 6809 is held in reset by a pullup attached to PB4 of the MCP23017.
        // Once our code has initialized and is ready to be accessed by the 6809 side of things
        // this reset condition is cleared and the 6809 is allowed to run. This has improved
        // the reliability of the startup code.
        if (IsReady == Ready)
        {
            // Clear 6809 reset
            Reset6809(false);
            IsReady = Running;
        }

        // Check / process command
        if(CmdSet)
        {
            DoCommand(D_CmdReg);
            CmdSet=false;
        }

        // check/process latched byte
        if(Latched)
        {
            InsertLatch(BytesLatch,D_LatchInReg);
            Latched=false;    
            ClearBusy();
        }

        // Check for interrupt from mcp23017, due to button press or card insert/removal
        // if so handle it.
        if (gpio_get(PORTB_INT_PIN))
        {
            poll_switches();
        }

        if(OldSAMBits != SAMBits)
        {
            ShowSAM();
        }

        if(TestW)
        {
            printfc(DEBUG_ENABLED,"TestByte=%0X\n",TestReg);
            TestW=false;
        }

        // check for a reset vector fetch by the 6809 and handle it.
        check_reset();
    }
}
