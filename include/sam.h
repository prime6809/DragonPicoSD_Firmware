#ifndef __SAM_H__
#define __SAM_H__

// SAM related stuff
#define SAM_BASE        0xFFC0
#define SAM_END         0xFFDF

// Mask to apply to SAM's address to extract the data bit
#define SAM_DATA_MASK   0x0001

// Mask to apply to SAM's address to select which bit to process.
#define SAM_ADDR_MASK   0x001E
#define SAM_ADDR_SHIFT  1

#define GetSAMBit(addr)         ((addr & SAM_ADDR_MASK) >> SAM_ADDR_SHIFT)
#define GetSAMData(addr)        (addr & SAM_DATA_MASK)
#define GetSAMDataMask(addr)    (1 << GetSAMBit(addr))

// Masks for SAMBits
#define SAM_TYPE_MASK   0x8000
#define SAM_TYPE_SHIFT  15

#define SAM_SIZE_MASK   0x6000
#define SAM_SIZE_SHIFT  13

#define SAM_RATE_MASK   0x1800
#define SAM_RATE_SHIFT 11

#define SAM_PAGE_MASK   0x0400
#define SAM_PAGE_SHIFT  10

#define SAM_MODE_MASK   0x0007
#define SAM_VADDR_MASK  0x03F8
#define SAM_VADDR_SHIFT 6

#define SAM_SG_MASK     0x0006
#define SAM_SG_SHIFT    1


#define GetSAMMapType(bits) ((bits & SAM_TYPE_MASK) >> SAM_TYPE_SHIFT)

#define GetSAMRAMSize(bits) ((bits & SAM_SIZE_MASK) >> SAM_SIZE_SHIFT)

#define GetSAMCPUSpeed(bits) ((bits & SAM_RATE_MASK) >> SAM_RATE_SHIFT)

#define GetSAMPage(bits) ((bits & SAM_PAGE_MASK) >> SAM_PAGE_SHIFT)


//Get SAM video mode, used to decode semigraphics modes.
#define GetSAMMode(bits)    (bits & SAM_MODE_MASK)

#define GetSAMSG(bits)      ((bits & SAM_SG_MASK) >> SAM_SG_SHIFT)

// Macros to get VDU memory base
#define GetVidMemBase(bits) ((bits & SAM_VADDR_MASK) << SAM_VADDR_SHIFT)
#define GetVidMemEnd(bits)  (GetVidMemBase(bits)+VID_MEM_SIZE)

#endif
