/**
 * @file
 * flash.h
 *
 * This file contains the function declarations for the Flashing functions
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
 * Author: Paul Zanna <paul@northboundnetworks.com>
 *		 & Kristopher Chen <Kristopher@northboundnetworks.com>
 *
 */


#ifndef FLASH_H_
#define FLASH_H_

void get_serial(uint32_t *uid_buf);
int firmware_check(void);
void firmware_upload(void);
void firmware_update(void);
void firmware_run(void);
void restart(void);
int flash_write_page(uint8_t *flash_page);
void firmware_buffer_init(void);
void firmware_store_init(void);
int xmodem_xfer(void);
void xmodem_clear_padding(uint8_t *buff);

// Verification testing commands
int write_verification(uint32_t location, uint64_t value);
int verification_check(void);

int test_write_command(uint32_t addr);
int test_erase_command(uint32_t addr);

struct verification_data
{
	uint32_t calculated;	// Last 4 bytes from summed data
	uint32_t found;			// 4 bytes at the end of uploaded firmware
};

#define X_EOT 0x04
#define X_ACK 0x06
#define X_NAK 0x15

#define ERASE_SECTOR_SIZE	65536
//#define NEW_FW_BASE			(IFLASH_ADDR + (5*IFLASH_NB_OF_PAGES/8)*IFLASH_PAGE_SIZE)
#define NEW_FW_MAX_SIZE		196608

#define SUCCESS 0
#define FAILURE 1

#define SKIP	0
#define UPDATE	1
#define RUN		2

#endif /* FLASH_H_ */