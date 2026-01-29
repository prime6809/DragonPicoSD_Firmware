;
; Simple MMC functions.
;
; 2011-06-13 Phill Harvey-Smith.
;
; 2024-09-28, Moved into own function so could be included in early
; part of rom that is the same for Dragon and CoCo. 
;
; Define MINIMUM to exclude code not needed by FLASH
;

;
; **** NOTE ****
;
; Send CMD_INIT_READ with MMC_sendCmdRaw *NOT* MMC_SendCmd
;
__mmc_simple_func

;
; Send Command
;
; Entry a = command to send
;
; exit a = status code.
;

MMC_SendCmd
		bsr		MMC_SendCmdRaw			; send the command

;		lda		#STATUS_FLAG_WRITTEN	; Written flag
;		anda	D_STATUS_REG			; is it set ?
;		beq		MMC_SendCmdNoResult		; no clear a and return

		lda		D_CMD_REG				; Get status (if any)
		tsta
		rts								; return
		
MMC_SendCmdNoResult

		clra							; flag no error
		rts
		
;
; Send a command but don't read a result.
;

MMC_SendCmdRaw
		sta		D_CMD_REG				; send the command

;
; MMC_WaitNotBusy, waits for the Pico busy flag to be reset.
;
MMC_WaitNotBusy
        ifndef  EMULATE
        
MMC_WaitNotBusyLoop        
		lda		D_STATUS_REG
;		jsr		CON_HexByteAt0		
		tsta
		bmi		MMC_WaitNotBusyLoop		; yes : keep waiting

        endc
		rts

;
; MMC_WaitPutLatchRead Send a byte in a to the AVR LATCH_REG and wait for it to read it
;
MMC_WaitPutLatchRead
		sta		D_LATCH_REG				; write the data
		bra		MMC_WaitNotBusy

;
; MMC_GetRAMCTRL
; 
; Return the current RAM_CTRL byte in a
;
MMC_GetRAMCTRL
		lda		#CMD_GET_RAM_CTRL		; Get the RAM_CTRL register
		
;
; MMC_SendCmdLatchRead
; 
; Send command in a, and get single byte reply (from latch) in a
;		
MMC_SendCmdLatchRead
		bsr		MMC_SendCmd				; send it
		
		lda		D_LATCH_REG				; retrieve it.
		rts

;
; MMC_SetRAMCTRL
; 
; Set the RAM_CTRL byte in to the value in a
;

MMC_SetRAMCTRL
		bsr		MMC_WaitPutLatchRead	; Put the new byte in the latch
		lda		#CMD_SET_RAM_CTRL		; Set it.
		bsr		MMC_SendCmd				; send it
		rts

__mmc_simple_func_end
