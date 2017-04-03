/**
 * @file
 * flash.c
 *
 * This file contains the function the Flashing functions
 *
 */

/*
 * This file is part of the Zodiac FX firmware.
 * Copyright (c) 2016 Northbound Networks.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Paul Zanna <paul@northboundnetworks.com>
 *		  & Kristopher Chen <Kristopher@northboundnetworks.com>
 *
 */

#include <asf.h>
#include <inttypes.h>
#include <string.h>
#include "flash.h"
#include "conf_bios.h"
#include "cmd_line.h"
#include "trace.h"

// Global variables
uint8_t shared_buffer[SHARED_BUFFER_LEN];
struct verification_data	verify;

extern bool bios_debug;

// Static variables
static uint32_t page_addr;
static	uint32_t ul_test_page_addr;
static	uint32_t ul_rc;
static	uint32_t ul_idx;
static	uint32_t ul_page_buffer[IFLASH_PAGE_SIZE / sizeof(uint32_t)];

/*
*	Get the unique serial number from the CPU
*
*/
void get_serial(uint32_t *uid_buf)
{
	uint32_t uid_ok = flash_read_unique_id(uid_buf, 4);
	return;
}

/*
*	Firmware update function
*
*/
void firmware_buffer_init(void)
{
	ul_test_page_addr = FLASH_BUFFER;
	
	/* Initialize flash: 6 wait states for flash writing. */
	ul_rc = flash_init(FLASH_ACCESS_MODE_128, 6);
	if (ul_rc != FLASH_RC_OK) {
		printf("Buffer initialization error %lu\n\r", (unsigned long)ul_rc);
		return;
	}
	
	// Unlock 8k lock regions (these should be unlocked by default)
	uint32_t unlock_address = ul_test_page_addr;
	while(unlock_address < FLASH_BUFFER_END)
	{
		ul_rc = flash_unlock(unlock_address,
		unlock_address + (4*IFLASH_PAGE_SIZE) - 1, 0, 0);
		if (ul_rc != FLASH_RC_OK)
		{
			printf("Buffer unlock error %lu\n\r", (unsigned long)ul_rc);
			return;
		}
		
		unlock_address += IFLASH_LOCK_REGION_SIZE;
	}

	// Erase 3 64k sectors
	uint32_t erase_address = ul_test_page_addr;
	while(erase_address < FLASH_BUFFER_END)
	{
		ul_rc = flash_erase_sector(erase_address);
		if (ul_rc != FLASH_RC_OK)
		{
			printf("Buffer erase error %lu\n\r", (unsigned long)ul_rc);
			return;
		}
		
		erase_address += ERASE_SECTOR_SIZE;
	}
	
	return;
}

/*
*	Firmware check function
*
*		Zodiac FX flash is broken into two main firmware regions:
*			- running firmware region
*			- update buffer region
*
*		This function checks the validity and state of each region,
*		with the following return values:
*			- 0 (SKIP): no action required - both regions either doesn't
*					exist, or are invalid (a manual firmware update is needed)
*			- 1 (UPDATE): update required - update buffer region is valid, and
*					 must be copied over the running firmware region
*			- 2 (RUN): run existing firmware - update buffer region either
*					 doesn't exist, or is invalid
*/
int firmware_check(void)
{
	unsigned long* firmware_pmem = (unsigned long*)FLASH_STORE;
	unsigned long* buffer_pmem = (unsigned long*)FLASH_BUFFER;
	
	if(*firmware_pmem == 0xFFFFFFFF)
	{
		// running firmware does not exist
		
		if(*buffer_pmem == 0xFFFFFFFF)
		{
			// update firmware does not exist
			
			return SKIP;
		}
		else
		{
			// update firmware exists
			
			if(verification_check() == SUCCESS)
			{
				// firmware is valid
			
				return UPDATE;
			}
			else
			{
				// firmware is invalid
				
				return SKIP;
			}
		}
	}
	else
	{
		// running firmware exists
		
		if(*buffer_pmem == 0xFFFFFFFF)
		{
			// update firmware does not exist
			
			return RUN;
		}
		else
		{
			// update firmware exists
			
			if(verification_check() == SUCCESS)
			{
				// firmware is valid
			
				return UPDATE;
			}
			else
			{
				// firmware is invalid
				
				return RUN;
			}
		}
		
	}
}


/*
*	Firmware update function
*
*/
void firmware_store_init(void)
{	
	ul_test_page_addr = FLASH_STORE;
	
	/* Initialize flash: 6 wait states for flash writing. */
	ul_rc = flash_init(FLASH_ACCESS_MODE_128, 6);
	if (ul_rc != FLASH_RC_OK) {
		printf("Firmware initialization error %lu\n\r", (unsigned long)ul_rc);
		return;
	}
	
	// Unlock 8k lock regions (these should be unlocked by default)
	uint32_t unlock_address = ul_test_page_addr;
	while(unlock_address < FLASH_STORE_END)
	{
		ul_rc = flash_unlock(unlock_address,
		unlock_address + (4*IFLASH_PAGE_SIZE) - 1, 0, 0);
		if (ul_rc != FLASH_RC_OK)
		{
			printf("Firmware unlock error %lu\n\r", (unsigned long)ul_rc);
			return;
		}
		
		unlock_address += IFLASH_LOCK_REGION_SIZE;
	}

	// Erase 3 64k sectors
	uint32_t erase_address = ul_test_page_addr;
	while(erase_address < FLASH_STORE_END)
	{
		ul_rc = flash_erase_sector(erase_address);
		if (ul_rc != FLASH_RC_OK)
		{
			printf("Firmware erase error %lu\n\r", (unsigned long)ul_rc);
			return;
		}
		erase_address += ERASE_SECTOR_SIZE;
	}
	
	return;
}

/*
*	Write a page to flash memory
*
*/
int flash_write_page(uint8_t *flash_page)
{
	if(ul_test_page_addr <= IFLASH_ADDR + IFLASH_SIZE - IFLASH_PAGE_SIZE)
	{
		ul_rc = flash_write(ul_test_page_addr, flash_page,
		IFLASH_PAGE_SIZE, 0);
	}
	else
	{
		// Out of flash range
		return 0;
	}

	if (ul_rc != FLASH_RC_OK)
	{
		return 0;
	}	
	
	ul_test_page_addr += IFLASH_PAGE_SIZE;
	
	return 1;
}

/*
*	Write a page to a specific address in flash memory
*
*/
int flash_write_page_s(uint8_t *flash_page, uint32_t address_s)
{
	if(address_s <= IFLASH_ADDR + IFLASH_SIZE - IFLASH_PAGE_SIZE)
	{
		ul_rc = flash_write(address_s, flash_page,
		IFLASH_PAGE_SIZE, 0);
	}
	else
	{
		// Out of flash range
		return 0;
	}

	if (ul_rc != FLASH_RC_OK)
	{
		return 0;
	}
		
	return 1;
}

/*
*	Copies firmware from buffer the run location
*
*/
void firmware_update(void)
{
	uint32_t update_address = FLASH_STORE;
	uint32_t buffer_address = FLASH_BUFFER;
	
	firmware_store_init();
	
	while(update_address < FLASH_STORE_END)
	{
		ul_rc = flash_write(update_address, buffer_address, IFLASH_PAGE_SIZE, 0);
		if (ul_rc != FLASH_RC_OK)
		{
			printf("-F- Flash programming error %lu\n\r", (unsigned long)ul_rc);
			return;
		}
		update_address += IFLASH_PAGE_SIZE;
		buffer_address += IFLASH_PAGE_SIZE;
	}
	return;
}

/*
*	Handle firmware update through CLI
*
*/
void firmware_upload(void)
{
	firmware_buffer_init();
	if(!xmodem_xfer())	// Receive new firmware image via XModem
	{
		printf("Error: failed to write firmware to memory\r\n");
	}
	return;
}

/*
*	Restart
*
*/
void restart(void)
{
	udc_detach();	// Detach the USB device before restart
	rstc_start_software_reset(RSTC);	// Software reset
	while (1);
}

/*
*	Runs the firmware
*
*/
void firmware_run(void)
{
	int i;
	// Pointer to the Application Section
	void (*firmware_code_entry)(void);
	   
	// Disable IRQ
	__disable_irq();
	
	// Clear pending IRQs
	for (i = 0; i < 8; i ++) NVIC->ICER[i] = 0xFFFFFFFF;
	for (i = 0; i < 8; i ++) NVIC->ICPR[i] = 0xFFFFFFFF;

	// Barriers
	__DSB();
	__ISB();
	
	// Rebase the Stack Pointer
	__set_MSP(*(uint32_t *) FLASH_STORE);
	
	// Change the vector table
	SCB->VTOR = ((uint32_t)FLASH_STORE & SCB_VTOR_TBLOFF_Msk);

     // Load the Reset Handler address of the application
     firmware_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)(FLASH_STORE + 4));
	 
	// Barriers
	__DSB();
	__ISB();

	//Enable interrupts
	__enable_irq();
    // Jump to user Reset Handler in the application
    firmware_code_entry();
}

/*
*	XModem transfer
*
*/
int xmodem_xfer(void)
{
	char ch;
	int timeout_clock = 0;
	int buff_ctr = 1;
	int byte_ctr = 1;
	int block_ctr = 0;
	uint8_t xmodem_crc = 0;
	
	// Prepare shared_buffer for storing each page of data
	memset(&shared_buffer, 0xFF, IFLASH_PAGE_SIZE);
	
	while(1)
	{
		while(udi_cdc_is_rx_ready())
		{
			ch = udi_cdc_getc();
			timeout_clock = 0;	// reset timeout clock
			
			// Check for <EOT>
			if (byte_ctr == 1 && ch == X_EOT)	// Note: byte_ctr is cleared to 0 and incremented in the previous loop
			{
				printf("%c", X_ACK);	// Send final <ACK>
				xmodem_clear_padding(&shared_buffer);	// strip the 0x1A fill bytes from the end of the last block
				
				// Send remaining data in buffer
				if(!flash_write_page(&shared_buffer))
				{
					return 0;
				}
				
				return 1;
			}
			else if(block_ctr == 4)
			{
				// Write the previous page of data
				if(!flash_write_page(&shared_buffer))
				{
					return 0;
				}
				
				// Reset block counter
				block_ctr = 0;
				
				// Reset buffer counter
				buff_ctr = 1;
				
				// Clear buffer to 0xFF for next page of data
				memset(&shared_buffer, 0xFF, IFLASH_PAGE_SIZE);
			}
			
			// Check for end of block
			if (byte_ctr == 132)
			{
				if (xmodem_crc == ch)		// Check CRC
				{
					printf("%c", X_ACK);	// If the CRC is OK then send a <ACK>
					block_ctr++;			// Increment block count
					byte_ctr = 0;			// Start a new 128-byte block
				}
				else
				{
					printf("%c", X_NAK);	// If the CRC is incorrect then send a <NAK>
					byte_ctr = 0;			// Start a new 128-byte block
					buff_ctr -= 128;		// Overwrite previous data
				}
				
				xmodem_crc = 0;				// Reset CRC
			}

			// Don't store the first 3 bytes <SOH>, <###>, <255-###>
			if (byte_ctr > 3)
			{
				shared_buffer[buff_ctr-1] = ch;	// Store received data
				buff_ctr++;
				xmodem_crc += ch;
			}
			
			byte_ctr++;
		}
		timeout_clock++;
		if (timeout_clock > 1000000)	// Timeout, send <NAK>
		{
			printf("%c", X_NAK);
			timeout_clock = 0;
		}
	}
}

/*
*	Remove XMODEM 0x1A padding at end of data
*
*/
xmodem_clear_padding(uint8_t *buff)
{
	int len = IFLASH_PAGE_SIZE;
	
	// Overwrite the padding element in the buffer (zero-indexed)
	while(len > 0)	// Move from end of buffer to beginning
	{
		if(buff[len-1] != 0xFF && buff[len-1] != 0x1A)
		{
			return;
		}
		else if(buff[len-1] == 0x1A)
		{
			// Latch onto 0x1A
			while(len > 0 && buff[len-1] == 0x1A)
			{
				// Write erase value
				buff[len-1] = 0xFF;
				len--;
			}
			
			return;
		}
		
		len--;
	}
	
	return;	// Padding characters removed
}

/*
*	Write test verification value to flash
*
*/
int write_verification(uint32_t location, uint64_t value)
{
	firmware_buffer_init();
	
	// Page to write to f/w
	uint8_t verification_page[IFLASH_PAGE_SIZE] = {0};
	
	// Copy verification value
	memcpy(verification_page, &value, sizeof(value));
	
	// Write specific page
	if(flash_write_page_s(verification_page, location)) return 1;
	// Write failed
	return 0;
}

int verification_check(void)
{
	char* fw_end_pmem	= (char*)FLASH_BUFFER_END;	// Buffer pointer to store the last address
	char* fw_step_pmem  = (char*)FLASH_BUFFER;		// Buffer pointer to the starting address
	uint32_t crc_sum	= 0;						// Store CRC sum
	uint8_t	 pad_error	= 0;						// Set when padding is not found
	
	/* Add all bytes of the uploaded firmware */
	// Decrement the pointer until the previous address has data in it (not 0xFF)
	while(*(fw_end_pmem-1) == '\xFF' && fw_end_pmem > FLASH_BUFFER)
	{
		fw_end_pmem--;
	}

	for(int sig=1; sig<=4; sig++)
	{
		if(*(fw_end_pmem-sig) != NULL)
		{
			TRACE("signature padding %d not found - last address: %08x", sig, fw_end_pmem);
			pad_error = 1;
		}
		else
		{
			TRACE("signature padding %d found", sig);
		}
	}
	
	// Start summing all bytes
	if(pad_error)
	{
		// Calculate CRC for debug
		while(fw_step_pmem < fw_end_pmem)
		{
			crc_sum += *fw_step_pmem;
			fw_step_pmem++;
		}
	}
	else
	{
		// Exclude CRC & padding from calculation
		while(fw_step_pmem < (fw_end_pmem-8))
		{
			crc_sum += *fw_step_pmem;
			fw_step_pmem++;
		}
	}
	
	TRACE("fw_step_pmem %08x; fw_end_pmem %08x;", fw_step_pmem, fw_end_pmem);
	
	// Update structure entry
	TRACE("CRC sum:   %04x", crc_sum);
	verify.calculated = crc_sum;
	
	/* Compare with last 4 bytes of firmware */
	// Get last 4 bytes of firmware	(4-byte CRC, 4-byte padding)
	verify.found = *(uint32_t*)(fw_end_pmem - 8);
	
	TRACE("CRC found: %04x", verify.found);
	
	// Compare calculated and found CRC
	if(verify.found == verify.calculated)
	{
		return SUCCESS;	
	}
	else
	{
		return FAILURE;
	}
int test_write_command(uint32_t addr)
{
	ul_test_page_addr = addr;
	
	for(uint16_t i=0;i<512;i++)
	{
		shared_buffer[i] = 'U';
	}
	
	for(uint16_t i=0;i<448;i++)
	{
		if(!flash_write_page(&shared_buffer))
		{
			printf("write error\r\n");
		}
	}
	
	return SUCCESS;
}

int test_erase_command(uint32_t addr)
{
	ul_test_page_addr = addr;

		
	/* Initialize flash: 6 wait states for flash writing. */
	ul_rc = flash_init(FLASH_ACCESS_MODE_128, 6);
	if (ul_rc != FLASH_RC_OK) {
		printf("Buffer initialization error %lu\n\r", (unsigned long)ul_rc);
		return FAILURE;
	}
		
	// Erase test
	ul_rc = flash_erase_page(ul_test_page_addr, IFLASH_ERASE_PAGES_32);
	if (ul_rc != FLASH_RC_OK)
	{
		printf("Buffer erase error %lu\n\r", (unsigned long)ul_rc);
		return FAILURE;
	}
	
	return SUCCESS;
}