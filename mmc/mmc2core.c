#include "ff.h"

#include "DragonPicoSD.h"
#include "mmc2def.h"
#include "mmc2.h"
#include "eeprom.h"
#include <stdint.h>
#include <stdio.h>

#include <hardware/gpio.h>

#define _INCLUDE_WFUNC_
#include "mmc2wfn.h"

#include <string.h>

#define LOG_CMD		1
#define LOG_DATAIN	0
#define LOG_LATCHED	0

//static BYTE LatchedData;

//static BYTE LatchedAddress;
//static BYTE LatchedAddressLast;

extern BYTE LastCommand;

#ifdef INCLUDE_SDDOS

extern BYTE globalCurDrive;
extern DWORD globalLBAOffset;
extern WORD globalCyl;         // Cylinder
extern BYTE globalHead;        // Head
extern BYTE globalSector;      // Sector no
extern BYTE FDCStatus;

extern void unmountImg(BYTE drive);

extern imgInfo driveInfo[];

#endif

extern unsigned char CardType;
extern unsigned char disk_initialize (BYTE);


// cache of the value written to port 0x0e
//BYTE byteValueLatch;

void log_cmd(BYTE	cmd)
{
#ifdef LOG_CMD
	switch(cmd)
	{
		case	CMD_DIR_OPEN: 
			printf("[%02X] CMD_DIR_OPEN\n",cmd); break;
		case	CMD_DIR_READ: 
			printf("[%02X] CMD_DIR_READ\n",cmd); break;
		case	CMD_DIR_CWD: 
			printf("[%02X] CMD_DIR_CWD\n",cmd); break;
		case	CMD_DIR_GETCWD: 
			printf("[%02X] CMD_DIR_GETCWD\n",cmd); break;
		case	CMD_DIR_MAKE: 
			printf("[%02X] CMD_DIR_MAKE\n",cmd); break;
		case	CMD_DIR_REMOVE: 
			printf("[%02X] CMD_DIR_REMOVE\n",cmd); break;
			
		case	CMD_DIR_SET_SNAPPATH: 
			printf("[%02X] CMD_DIR_SET_SNAPPATH\n",cmd); break;
		case	CMD_DIR_GET_SNAPPATH: 
			printf("[%02X] CMD_DIR_GET_SNAPPATH\n",cmd); break;

		case	CMD_FILE_CLOSE: 
			printf("[%02X] CMD_FILE_CLOSE\n",cmd); break;
		case	CMD_FILE_OPEN_READ: 
			printf("[%02X] CMD_FILE_OPEN_READ\n",cmd); break;
		case	CMD_FILE_OPEN_IMG: 
			printf("[%02X] CMD_FILE_OPEN_IMG\n",cmd); break;
		case	CMD_FILE_OPEN_WRITE: 
			printf("[%02X] CMD_FILE_OPEN_WRITE\n",cmd); break;
		case	CMD_FILE_DELETE: 
			printf("[%02X] CMD_FILE_DELETE\n",cmd); break;
		case	CMD_FILE_GETINFO: 
			printf("[%02X] CMD_FILE_GETINFO\n",cmd); break;
		case	CMD_FILE_OPENAUTOD: 
			printf("[%02X] CMD_FILE_OPENAUTOD\n",cmd); break;
		case	CMD_FILE_OPENAUTOC: 
			printf("[%02X] CMD_FILE_OPENAUTOC\n",cmd); break;		
		case	CMD_FILE_OPEN_OVERWRITE: 
			printf("[%02X] CMD_FILE_OPEN_OVERWRITE\n",cmd); break;
		case	CMD_FILE_OPEN_SNAPR: 
			printf("[%02X] CMD_FILE_OPEN_SNAPR\n",cmd); break;
		case	CMD_FILE_OPEN_SNAPW: 
			printf("[%02X] CMD_FILE_OPEN_SNAPW\n",cmd); break;
		case	CMD_FILE_OPEN_STREAMR: 
			printf("[%02X] CMD_FILE_OPEN_STREAMR\n",cmd); break;
		case	CMD_FILE_OPEN_STREAMW: 
			printf("[%02X] CMD_FILE_OPEN_STREAMW\n",cmd); break;
		case	CMD_FILE_COPY: 
			printf("[%02X] CMD_FILE_COPY\n",cmd); break;
		case	CMD_FILE_RENAME: 
			printf("[%02X] CMD_FILE_RENAME\n",cmd); break;
		case 	CMD_FILE_OPENCRE_IMG:
			printf("[%02X] CMD_FILE_OPENCRE_IMG\n",cmd); break;
		
		case	CMD_INIT_READ: 
			printf("[%02X] CMD_INIT_READ\n",cmd); break;
		case	CMD_INIT_WRITE: 
			printf("[%02X] CMD_INIT_WRITE\n",cmd); break;

		case	CMD_READ_BYTES: 
			printf("[%02X] CMD_READ_BYTES\n",cmd); break;
		case	CMD_WRITE_BYTES: 
			printf("[%02X] CMD_WRITE_BYTES\n",cmd); break;
		case	CMD_REWIND: 
			printf("[%02X] CMD_REWIND\n",cmd); break;
		case	CMD_SEEK: 
			printf("[%02X] CMD_SEEK\n",cmd); break;
		case	CMD_TELL: 
			printf("[%02X] CMD_TELL\n",cmd); break;
		case	CMD_GET_FID: 
			printf("[%02X] CMD_GET_FID\n",cmd); break;


		case	CMD_GET_STRLEN: 
			printf("[%02X] CMD_GET_STRLEN\n",cmd); break;
		case	CMD_WRITE_IOREG: 
			printf("[%02X] CMD_WRITE_IOREG\n",cmd); break;
		case	CMD_READ_IOREG: 
			printf("[%02X] CMD_READ_IOREG\n",cmd); break;
		case	CMD_EXEC_PACKET: 
			printf("[%02X] CMD_EXEC_PACKET\n",cmd); break;
		
		case	CMD_LOAD_LBA: 
			printf("[%02X] CMD_LOAD_LBA\n",cmd); break;
		case	CMD_GET_IMG_STATUS: 
			printf("[%02X] CMD_GET_IMG_STATUS\n",cmd); break;
		case	CMD_GET_IMG_NAME: 
			printf("[%02X] CMD_GET_IMG_NAME\n",cmd); break;
		case	CMD_READ_IMG_SEC: 
			printf("[%02X] CMD_READ_IMG_SEC\n",cmd); break;
		case	CMD_WRITE_IMG_SEC: 
			printf("[%02X] CMD_WRITE_IMG_SEC\n",cmd); break;
		case	CMD_SER_IMG_INFO: 
			printf("[%02X] CMD_SER_IMG_INFO\n",cmd); break;
		case	CMD_VALID_IMG_NAMES: 
			printf("[%02X] CMD_VALID_IMG_NAMES\n",cmd); break;
		case	CMD_IMG_UNMOUNT: 
			printf("[%02X] CMD_IMG_UNMOUNT\n",cmd); break;
		case	CMD_IMG_SEEK: 
			printf("[%02X] CMD_IMG_SEEK\n",cmd); break;
		case	CMD_CREATE_IMG: 
			printf("[%02X] CMD_CREATE_IMG\n",cmd); break;
	    case	CMD_GET_FDC_STATUS: 
			printf("[%02X] CMD_GET_FDC_STATUS\n",cmd); break;
	    case	CMD_READ_NEXT_IMG_SEC: 
			printf("[%02X] CMD_READ_NEXT_IMG_SEC\n",cmd); break;
	    case	CMD_LOAD_HR: 
			printf("[%02X] CMD_LOAD_HR\n",cmd); break;

		case	CMD_CAS_FTYPE: 
			printf("[%02X] CMD_CAS_FTYPE\n",cmd); break;
        case    CMD_CAS_EMULATE:
        	printf("[%02X] CMD_CAS_EMULATE\n",cmd); break;
        case    CMD_CAS_EMULATE2:
        	printf("[%02X] CMD_CAS_EMULATE2\n",cmd); break;
        
		case	CMD_GET_CARD_TYPE: 
			printf("[%02X] CMD_GET_CARD_TYPE\n",cmd); break;
		case	CMD_SET_BUSY: 
			printf("[%02X] CMD_SET_BUSY\n",cmd); break;
		case	CMD_NOP: 
			printf("[%02X] CMD_NOP\n",cmd); break;
		case	CMD_SYNC: 
			printf("[%02X] CMD_SYNC\n",cmd); break;

		case	CMD_CKSUM: 
			printf("[%02X] CMD_CKSUM\n",cmd); break;


		case	CMD_GET_DATETIME: 
			printf("[%02X] CMD_GET_DATETIME\n",cmd); break;  
		case	CMD_SET_DATETIME: 
			printf("[%02X] CMD_SET_DATETIME\n",cmd); break;

		case	CMD_GET_FW_VER: 
			printf("[%02X] CMD_GET_FW_VER\n",cmd); break;
		case	CMD_GET_BL_VER: 
			printf("[%02X] CMD_GET_BL_VER\n",cmd); break;


		case	CMD_GET_RAM_CTRL:
			printf("[%02X] CMD_GET_RAM_CTRL\n",cmd); break;
		case	CMD_SET_RAM_CTRL:
			printf("[%02X] CMD_SET_RAM_CTRL\n",cmd); break;


		case	CMD_GET_SAMREGS:
			printf("[%02X] CMD_GET_SAMREGS\n",cmd); break;

		case	CMD_GET_CFG_BYTE: 
			printf("[%02X] CMD_GET_CFG_BYTE\n",cmd); break;
		case	CMD_SET_CFG_BYTE: 
			printf("[%02X] CMD_SET_CFG_BYTE\n",cmd); break;
		case	CMD_SET_PLATFORM: 
			printf("[%02X] CMD_SET_PLATFORM\n",cmd); break;
		case	CMD_READ_AUX: 
			printf("[%02X] CMD_READ_AUX\n",cmd); break;
		case	CMD_GET_HEARTBEAT: 
			printf("[%02X] CMD_GET_HEARTBEAT\n",cmd); break;
			
		default : 
			printf("[%02X] Unknown\n",cmd); 
	}
#endif

#if	LOG_LATCHED
	printf("Latched count=%d\n",BytesLatch.Index);
	for(int Idx=0; Idx < BytesLatch.Index; Idx++)
		printf("%02X ",BytesLatch.Bytes[Idx]);

	printf("\n");
#endif

}

void log_status(uint8_t	status)
{
	if (status & STATUS_ERROR)
	{
		printf("Error:");

		status &= ~STATUS_ERROR;

		switch(status)
		{
			// FATFS errors.
			case FR_OK :
				printf("[%02X] OK (FATFS) or .",status); break;
			case FR_DISK_ERR :
				printf("[%02X] Disk error.",status); break;
			case FR_INT_ERR :
				printf("[%02X] Assertion failed.",status); break;
			case FR_NOT_READY :
				printf("[%02X] Not ready.",status); break;
			case FR_NO_FILE :
				printf("[%02X] File not found.",status); break;
			case FR_NO_PATH :
				printf("[%02X] Path not found.",status); break;
			case FR_INVALID_NAME :
				printf("[%02X] Invalid path name / format.",status); break;
			case FR_DENIED :
				printf("[%02X] Access denied or dir full.",status); break;
			case FR_EXIST :
				printf("[%02X] Access denied, prohibited access.",status); break;
			case FR_INVALID_OBJECT :
				printf("[%02X] File/directory object invalid.",status); break;
			case FR_WRITE_PROTECTED :
				printf("[%02X] Physical drive write protected.",status); break;
			case FR_INVALID_DRIVE :
				printf("[%02X] Invalid drive.",status); break;
			case FR_NOT_ENABLED :
				printf("[%02X] Volume has no work area.",status); break;
			case FR_NO_FILESYSTEM :
				printf("[%02X] No valid FAT / FAT32 / FATX filesystem",status); break;
			case FR_MKFS_ABORTED :
				printf("[%02X] Could not make filesystem.",status); break;
			case FR_TIMEOUT :
				printf("[%02X] Could not get access to volume in desired period.",status); break;
			case FR_LOCKED :
				printf("[%02X] Operation not permitted due to file sharing.",status); break;
			case FR_NOT_ENOUGH_CORE :
				printf("[%02X] The LFN working buffer could not be allocated.",status); break;
			case FR_TOO_MANY_OPEN_FILES :
				printf("[%02X] Number of files open > FF_FS_LOCK.",status); break;
			case FR_INVALID_PARAMETER :
				printf("[%02X] Given parameter(s) invalid (FATFS).",status); break;

			// DragonMMC / DragonPicoSD errors
			case ERROR_INVALID_CMD :
				printf("[%02X] Invalid command code.",status); break;
			case ERROR_INVALID_IMAGE :
				printf("[%02X] Invalid image.",status); break;
			case ERROR_NO_DATA :
				printf("[%02X] No Data",status); break;
			case ERROR_INVALID_DRIVE :
				printf("[%02X] Invalid DragonDOS / RSDOS drive number.",status); break;
			case ERROR_READ_ONLY :
				printf("[%02X] Emulated drive write protected.",status); break;
			case ERROR_ALREADY_MOUNT :
				printf("[%02X] Disk image already mounted (in another drive).",status); break;
			case ERROR_INVALID_TIME :
				printf("[%02X] Invalid date/time format specified.",status); break;
			case ERROR_INVALID_FID :
				printf("[%02X] Invalid File ID specified.",status); break;
			case ERROR_INVALID_PARAM :
				printf("[%02X] Invalid parameter.",status); break;
			case ERROR_NO_FREE_FID :
				printf("[%02X] No free file IDs.",status); break;
			case ERROR_INVALID_PLATFORM :
				printf("[%02X] Invalid platform specified.",status); break;
		}
	} 
	else 	if (status & STATUS_COMPLETE)
	{
		printf("Complete:");

		status &= ~STATUS_COMPLETE;

		switch (status)
		{
			case STATUS_LAST :
				printf("[%02X] Last result.",status); break;
		}
	}
	printf("\n");
}

uint32_t CalcChecksum(uint8_t *Buff, 
					  uint32_t Size)
{
	uint32_t	CkSum = 0;
	uint32_t	Idx;

	for(Idx=0; Idx < Size; Idx++)
		CkSum=CkSum+Buff[Idx];

	return CkSum;
}

void DoChecksum()
{
	printf("ReadRam checksum  : %08lX\n",CalcChecksum((uint8_t *)ReadRAM,VISIBLE_ALLROM_SIZE));
	printf("WriteRam checksum : %08lX\n",CalcChecksum((uint8_t *)WriteRAM,VISIBLE_ALLROM_SIZE));
}

// Do a command, *ALL* commands should return a status, even if it is STATUS_COMPLETE
// as the pico implementation does not have some of the limitations of the AVR
// implementaton, there is no reason to not set a status even if it is ignored
// by the client code!
// All worker functions should return a uint8_t status also.
// Moved status returning and Busy clearing code to this function, since all
// commands operate through this routine it should do the Status//cleanup functions.
void DoCommand(uint8_t Cmd)
{
	uint8_t (*worker)(void) = NULL;
    uint8_t	CmdStatus = STATUS_COMPLETE;	// default status complete

    static unsigned char heartbeat = 0x55;
    uint8_t EEData;

    BYTE EEAddress;

	LastCommand=Cmd;

#if LOG_CMD
	if(DEBUG_ENABLED)
		log_cmd(Cmd);
#endif

	// Directory group, moved here 2011-05-29 PHS.
	//
	if (Cmd == CMD_DIR_OPEN)
	{
		// reset the directory reader
		worker = WFN_DirectoryOpen;
	}
	else if (Cmd == CMD_DIR_READ)
	{
		// get next directory entry
		worker = WFN_DirectoryRead;
	}
	else if (Cmd == CMD_DIR_CWD)
	{
		// set CWD
		worker = WFN_SetCWDirectory;
	}
	else if (Cmd == CMD_DIR_GETCWD)
	{
		// Get CWD
		worker = WFN_GetCWDirectory;
	}	
	else if (Cmd == CMD_DIR_MAKE)
	{
		worker = WFN_MakeDirectory;
	}
	else if (Cmd == CMD_DIR_REMOVE)
	{
		worker = WFN_RemoveDirectory;
	}

	else if (Cmd == CMD_DIR_SET_SNAPPATH)
	{
		worker = WFN_SetSnapPath;
	}
	else if (Cmd == CMD_DIR_GET_SNAPPATH)
	{
		// copy snap path to global data and return it
		strncpy((char *)globalData,SnapPath,SNAP_PATH_LEN);
		CmdStatus=STATUS_COMPLETE;
	}

	// File group.
	//
	else if (Cmd == CMD_FILE_CLOSE)
	{
		// close the open file, flushing any unwritten data
		//
		worker = WFN_FileClose;
	}
	else if (Cmd == CMD_FILE_OPEN_READ)
	{
		// open the file with name in global data buffer
		//
		worker = WFN_FileOpenRead;
	}
#ifdef INCLUDE_SDDOS
	else if (Cmd == CMD_FILE_OPEN_IMG)
	{
		// open the file as backing image for virtual floppy drive
		// drive number in globaldata[0] followed by asciiz name.
		//
		worker = WFN_OpenSDDOSImg;
	}
#endif
	else if (Cmd == CMD_FILE_OPEN_WRITE)
	{
		// open the file with name in global data buffer for write
		//
		worker = WFN_FileOpenWrite;
	}
	else if (Cmd == CMD_FILE_DELETE)
	{
		// delete the file or directory with name in global data buffer
		//
		worker = WFN_FileDelete;
	}
	else if (Cmd == CMD_FILE_GETINFO)
	{
		// return file's status byte
		//
		worker = WFN_FileGetInfo;
	}
	else if (Cmd == CMD_FILE_OPENAUTOD)
	{
		worker = WFN_FileOpenAuto;
	}  
	else if (Cmd == CMD_FILE_OPENAUTOC)
	{
		worker = WFN_FileOpenAuto;
	}  
	else if (Cmd == CMD_FILE_OPEN_OVERWRITE)
	{
		// open a file for overwrite
		//
		worker = WFN_FileOpenOverwrite;
	}     
	else if (Cmd == CMD_FILE_OPEN_SNAPR)
	{
		// return file's status byte
		//
		worker = WFN_FileOpenSnapR;
	}     
	else if (Cmd == CMD_FILE_OPEN_SNAPW)
	{
		// return file's status byte
		//
		worker = WFN_FileOpenSnapW;
	}  
	else if (Cmd == CMD_FILE_OPEN_STREAMR)
	{
		// Open a file for streaming from, currently a NOP :)
		//
	}     
	else if (Cmd == CMD_FILE_OPEN_STREAMW)
	{
		// Open a file for streaming to
		//
		worker = WFN_FileOpenStreamW;
	}  
	else if (Cmd == CMD_FILE_COPY)
	{
		// Copy a file to another file
		//
		worker = WFN_FileCopyRename;
	}  
	else if (Cmd == CMD_FILE_RENAME)
	{
		// Rename a file
		//
		worker = WFN_FileCopyRename;
	}  

#ifdef INCLUDE_SDDOS
	else if (Cmd == CMD_FILE_OPENCRE_IMG)
	{
		// open the file as backing image for virtual floppy drive
		// drive number in globaldata[0] followed by asciiz name.
		// This will create a new image if it does not exist
		worker = WFN_OpenSDDOSImgCreate;
	}
#endif

	else if (Cmd == CMD_INIT_READ)
	{
		// All data read requests must send CMD_INIT_READ before beggining reading
		// data from READ_DATA_PORT. After execution of this command the first byte 
		// of data may be read from the READ_DATA_PORT.
		//
		globalIndex = 0;
		ReadRegs[D_OS_DATA_REG]=globalData[0];
	}
	else if (Cmd == CMD_INIT_WRITE)
	{
		// all data write requests must send CMD_INIT_WRITE here before writing data at 
		// WRITE_DATA_REG	
		//
		globalIndex = 0;
		CmdStatus=STATUS_COMPLETE;
	}
	else if (Cmd == CMD_READ_BYTES)
	{	
		worker = WFN_FileRead;
	}
	else if (Cmd == CMD_WRITE_BYTES)
	{
		worker = WFN_FileWrite;
	}
	else if (Cmd == CMD_REWIND)
	{
		worker = WFN_FileRewind;
	}
	else if (Cmd == CMD_SEEK)
	{
		worker = WFN_FileSeek;
	}
	else if (Cmd == CMD_TELL)
	{
		worker = WFN_FileTell;
	}
	else if (Cmd == CMD_GET_FID)
	{
		worker = WFN_GetFreeFID;
	}
	

	// 
	// Utility functions
	else if(Cmd == CMD_GET_STRLEN)
	{
		printfc(DEBUG_ENABLED,"strlen=%d\n",strlen((const char*)globalData));
		WriteLatchOut(strlen((const char*)globalData));
	}
	else if(Cmd == CMD_WRITE_IOREG)
	{
		worker = WFN_WriteIO;
	}
	else if(Cmd == CMD_READ_IOREG)
	{
		worker = WFN_ReadIO;
	}	
	// 
	// Exec a packet in the data buffer.
	else if (Cmd == CMD_EXEC_PACKET)
	{
		worker = WFN_ExecuteArbitrary;
	}

#ifdef INCLUDE_SDDOS
	// SDDOS/LBA operations
	//
	else if (Cmd == CMD_LOAD_LBA)
	{
		// load sddos parameters for read/write
		// globalData[0] = img num
		// globalData[1..4 incl] = 256 byte sector number
		globalCurDrive = globalData[0] & 3;
		SET_LBAMODE(globalCurDrive);
	
		driveInfo[globalCurDrive].CurrentLBA = load_lsn((BYTE *)&globalData[1]);
		printfc(DEBUG_ENABLED,"driveInfo[%d].CurrentLBA=%lu\n",globalCurDrive,driveInfo[globalCurDrive].CurrentLBA);
	}
	else if (Cmd == CMD_GET_IMG_STATUS)
	{
		// retrieve sddos image status
		// globalData[0] = img num		
		//  -- maybe convert to latch out?
		WriteDataPort(driveInfo[BytesLatch.Bytes[0] & 3].attribs);
	}
	else if (Cmd == CMD_GET_IMG_NAME)
	{
		// retrieve sddos image names
		//
		worker = WFN_GetSDDOSImgNames;
	}
	else if (Cmd == CMD_READ_IMG_SEC)
	{
		// read image sector
		//
		worker = WFN_ReadSDDOSSect;
	}
	else if (Cmd == CMD_WRITE_IMG_SEC)
	{
		// write image sector
		//
		worker = WFN_WriteSDDOSSect;
	}
	else if (Cmd == CMD_SER_IMG_INFO)
	{
		// serialise the current image information
		//
		worker = WFN_SerialiseSDDOSDrives;
	}
	else if (Cmd == CMD_VALID_IMG_NAMES)
	{
		// validate the current sddos image names
		//
		worker = WFN_ValidateSDDOSDrives;
	}
	else if (Cmd == CMD_IMG_UNMOUNT)
	{
		// unmount the selected drive
		//
		worker = WFN_UnmountSDDOSImg;
	}
	else if (Cmd == CMD_IMG_SEEK)
	{
		// Try to seek the image
		//
		worker = WFN_ImgSeek;
	}
	else if (Cmd == CMD_CREATE_IMG)
	{
		// Create a new image or re-initialize an existing one
		// drive and mas LSN must be supplied by calling 
		// CMD_LOAD_PARAM first
		//
		worker = WFN_CreateImg;
	}
	else if (Cmd == CMD_GET_FDC_STATUS)
	{
		printfc(DEBUG_ENABLED,"FDCStatus:%02X\n",FDCStatus);
		CmdStatus=FDCStatus;
	}	
	else if (Cmd == CMD_READ_NEXT_IMG_SEC)
	{
		// read image sector
		//
		worker = WFN_ReadSDDOSNextSec;
	}
	else if (Cmd == CMD_LOAD_HR)
	{
		// globalData[] :
		// 0    : Image number 0..3 (currently)
		// 1    : Head no
		// 2    : Sector no (or sectors / track for format)
		globalCurDrive                          = globalData[0] & 3;
		driveInfo[globalCurDrive].CurrentHead   = globalData[1];
		driveInfo[globalCurDrive].CurrentSecNo  = globalData[2];
		SET_CHRNMODE(globalCurDrive);
		printfc(DEBUG_ENABLED,"CMD_LOAD_HR: Drive=%d, Head=%d, Sector=%d\n",
				globalCurDrive, driveInfo[globalCurDrive].CurrentHead,  driveInfo[globalCurDrive].CurrentSecNo);
	}

#endif
	// 
	// Cassette file handling commands
	//
	else if (Cmd == CMD_CAS_FTYPE)
	{
		worker = WFN_GetCasFiletype;
	}

	//
	// Utility commands.
	// Moved here 2011-05-29 PHS
	else if (Cmd == CMD_GET_CARD_TYPE)
	{
		// get card type - it's a slowcmd despite appearance
		disk_initialize(0);
		WriteDataPort(CardType);
	}
	else if (Cmd == CMD_SET_BUSY)
	{
		// Just returns with busy set for testing.
	}
	else if (Cmd == CMD_NOP)
	{
		printf("SAMBits=%04X\n",SAMBits);
	}
	else if (Cmd == CMD_SYNC)
	{
		worker = WFN_SyncDisk;
	}
	else if (Cmd == CMD_CKSUM)
	{
		DoChecksum();
	}
	else if (Cmd == CMD_GET_DATETIME)
	{
		worker = WFN_GetDateTime;
	}
	else if (Cmd == CMD_SET_DATETIME)
	{
		worker = WFN_SetDateTime;
	}
	else if (Cmd == CMD_GET_FW_VER) // read firmware version
	{
		WriteLatchOut(VSN_MAJ<<4|VSN_MIN);
		strcpy((char *)&globalData[0],CompileTime);
	}
	else if (Cmd == CMD_GET_BL_VER) // read bootloader version
	{
		WriteLatchOut(blVersion);
		strcpy((char *)&globalData[0],"No bootloader");
	}
	else if (Cmd == CMD_SET_RAM_CTRL)	// Set RAM control reg
	{
		worker = WFN_SetRAMCtrl;
	}
	else if (Cmd == CMD_GET_RAM_CTRL)	// Get RAM control reg
	{
		worker = WFN_GetRAMCtrl;
	}
	else if (Cmd == CMD_GET_SAMREGS)	// Return SAM regs
	{
		globalData[0]=((SAMBits >> 8) & 0xFF);
		globalData[1]=(SAMBits & 0xFF);
	}
	else if (Cmd == CMD_GET_CFG_BYTE) // read config byte
	{
		EEAddress=BytesLatch.Bytes[0];
		read_ee(EE_ADDRESS,EE_SYSFLAGS+EEAddress,&EEData);
		WriteDataPort(EEData);
		printfc(DEBUG_ENABLED,"Get Config:Platform=%02X, Config=%02X\n",EEAddress,EEData);
	}
	else if (Cmd == CMD_SET_CFG_BYTE) // write config byte
	{
		if (1 < BytesLatch.Index)
		{
			EEAddress=BytesLatch.Bytes[0];
			EEData=BytesLatch.Bytes[1];
	
			printfc(DEBUG_ENABLED,"set config:platform=%02X, Config=%02X\n",EEAddress,EEData);
			
			write_ee(EE_ADDRESS,EE_SYSFLAGS+EEAddress,EEData);
			printfc(DEBUG_ENABLED,"set config: eeprom written\n");
			
			if ((PLAT_SYSTEM == EEAddress))           
				configByte=EEData;
		}

		CmdStatus = (1 < BytesLatch.Index) ?  STATUS_COMPLETE : (STATUS_ERROR | ERROR_INVALID_PARAM);
	}
	else if (Cmd == CMD_SET_PLATFORM)
	{
		if (0 < BytesLatch.Index)
		{
			if (PlatformValid(BytesLatch.Bytes[0]))
				globalPlatform = BytesLatch.Bytes[0];
			else
				globalPlatform = PLAT_INVALID;
		}
		printfc(DEBUG_ENABLED,"Platform set to %02X\n",globalPlatform);
		
		// Load the settings here, as the rom always sets the platform when booting, that way
		// we don't have to eject and insert the card to pick these up.
		LoadSettings();
		
		CmdStatus= PlatformValid(globalPlatform) ? STATUS_COMPLETE : (STATUS_ERROR | ERROR_INVALID_PLATFORM);  
	}
	else if (Cmd == CMD_READ_AUX) // read porta - latch & aux pin on dongle
	{
		//WriteDataPort(LatchedAddress);
	}
	else if (Cmd == CMD_GET_HEARTBEAT)
	{
		// get heartbeat - this may be important as we try and fire
		// an irq very early to get the OS hooked before its first
		// osrdch call. the psp may not be enabled by that point,
		// so we have to wait till it is.
		//
		WriteDataPort(heartbeat);
		heartbeat ^= 0xff;
	}
	else	// Invalid command 
	{
		//WriteStatus(STATUS_ERROR | ERROR_INVALID_CMD);
		CmdStatus=STATUS_ERROR | ERROR_INVALID_CMD;
	}
	
	if (worker)
	{
		CmdStatus=worker();
	}

	// We have to do this here, so that the worker call gets a chance to
	// see the latched bytes before clearing.
	// Don't reset latch if writing data.
	if (CMD_INIT_WRITE != LastCommand)
		ResetLatch(BytesLatch);

	if(DEBUG_ENABLED)
		log_status(CmdStatus);
	
	WriteStatus(CmdStatus);

	ClearBusy();
}

