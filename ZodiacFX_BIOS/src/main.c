/**
 * @file
 * main.c
 *
 * This file contains the main loop for Zodiac FX BIOS
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
 *
 */


#include <asf.h>
#include <string.h>

#include "cmd_line.h"
#include "flash.h"


// Global variables
int charcount, charcount_last;
uint32_t uid_buf[4];

/*
*	This function is where bad code goes to die!
*	Hard faults are trapped here and won't return.
*
*/
void HardFault_Handler(void)
{
	while(1);
}

/*
*	Main program loop
*
*/
int main (void)
{	 
	unsigned long* firmware_pmem = (unsigned long*)FLASH_STORE;
	unsigned long* buffer_pmem = (unsigned long*)FLASH_BUFFER;
	
	if(memcmp(firmware_pmem, buffer_pmem, 4) == 0 && *firmware_pmem != 0xFFFFFFFF)		// If both location are the same and not blank then run firmware
	{
		firmware_run();
	}
	
	if(memcmp(firmware_pmem, buffer_pmem, 4) != 0 && *buffer_pmem != 0xFFFFFFFF)		// If the buffers are different then there must be a new version, update and run
	{
		firmware_update();
		firmware_run();
	}
	  
	uint32_t wdt_mode, timeout_value;
	char cCommand[64];
	char cCommand_last[64];
	memset(&cCommand, 0, sizeof(cCommand));
	memset(&cCommand_last, 0, sizeof(cCommand_last));
	cCommand[0] = '\0';
	charcount = 0;
	
	sysclk_init();
	board_init();

	wdt_init(WDT, wdt_mode, timeout_value, timeout_value);
	wdt_disable(WDT);

	irq_initialize_vectors(); // Initialize interrupt vector table support.
	cpu_irq_enable(); // Enable interrupts
	stdio_usb_init();
	
	while(1)
	{
		task_command(cCommand, cCommand_last);
	}
}
