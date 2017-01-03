/**
 * @file
 * cmd_line.c
 *
 * This file contains the command line functions
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
#include <stdlib.h>
#include <inttypes.h>
#include "conf_bios.h"
#include "cmd_line.h"
#include "flash.h"


#define RSTC_KEY  0xA5000000

// Global variables
extern int charcount, charcount_last;

// Local Variables
bool showintro = true;
uint8_t esc_char = 0;
uint8_t uCLIContext = 0;

// Internal Functions
void command_root(char *command, char *param1, char *param2, char *param3);
void printintro(void);
void printhelp(void);


/*
*	Main command line loop
*
*	@param str - pointer to the current command string
*	@param str_last - pointer to the last command string
*/
void task_command(char *str, char *str_last)
{
	char ch;
	char *command;
	char *param1;
	char *param2;
	char *param3;
	char *pch;

	while(udi_cdc_is_rx_ready()){
		ch = udi_cdc_getc();


		if (showintro == true)	// Show the intro only on the first key press
		{
			printintro();
			showintro = false;
			ch = 13;
		}
		if (ch == 27) // Is this the start of an escape key sequence?
		{
			esc_char = 1;
			return;
		}
		if (ch == 91 && esc_char == 1) // Second key in the escape key sequence?
		{
			esc_char = 2;
			return;
		}
		if (ch == 65 && esc_char == 2 && charcount == 0)	// Last char for the escape sequence for the up arrow (ascii codes 27,91,65)
		{
			strcpy(str, str_last);
			charcount = charcount_last;
			printf("%s",str);
			esc_char = 0;
			return;
		}

		if (ch == 13)	// Enter Key
		{
			printf("\r\n");
			str[charcount] = '\0';
			strcpy(str_last, str);
			charcount_last = charcount;
			pch = strtok (str," ");
			command = pch;
			pch = strtok (NULL, " ");
			param1 = pch;
			pch = strtok (NULL, " ");
			param2 = pch;
			pch = strtok (NULL, " ");
			param3 = pch;

			if (charcount > 0)
			{
				command_root(command, param1, param2, param3);
			}

			printf("Bootloader# ");

			charcount = 0;
			str[0] = '\0';
			esc_char = 0;
			return;

		} else if ((ch == 127 || ch == 8) && charcount > 0)	// Backspace key
		{
			charcount--;
			char tempstr[64];
			tempstr[0] = '\0';
			strncat(tempstr,str,charcount);
			strcpy(str, tempstr);
			printf("%c",ch); // echo to output
			esc_char = 0;
			return;

		} else if (charcount < 63 && ch > 31 && ch < 127 && esc_char == 0)	// Alphanumeric key
		{
			strncat(str,&ch,1);
			charcount++;
			printf("%c",ch); // echo to output
			esc_char = 0;
			return;
		}

		if (esc_char > 0) esc_char = 0; // If the escape key wasn't up arrow (ascii 65) then clear to flag
	}
}

/*
*	Commands within the root context
*
*	@param command - pointer to the command string
*	@param param1 - pointer to parameter 1
*	@param param2- pointer to parameter 2
*	@param param2 - pointer to parameter 3
*/
void command_root(char *command, char *param1, char *param2, char *param3)
{

	// Upload firmware
	if (strcmp(command, "upload") == 0)
	{
		printf("Please begin firmware upload\r\n");
		firmware_upload();		
		return;

	}

	// Update firmware
	if (strcmp(command, "update") == 0)
	{
		printf("Please begin firmware upload\r\n");
		firmware_update();
		return;

	}

	// Run firmware
	if (strcmp(command, "run") == 0)
	{
		firmware_run();
		return;
	}
			
	// Display help
	if (strcmp(command, "help") == 0)
	{
		printhelp();
		return;
	}

	// Restart switch
	if (strcmp(command, "restart")==0)
	{
		printf("Restarting the Zodiac FX, please reopen your terminal application.\r\n");
		for(int x = 0;x<100000;x++);	// Let the above message get sent to the terminal before detaching
		udc_detach();	// Detach the USB device before restart
		rstc_start_software_reset(RSTC);	// Software reset
		while (1);
	}

	// Display firmware version
	if (strcmp(command, "status") == 0)
	{
		int buffer_size = 0;
		// check flash contents
		unsigned long* firmware_pmem = (unsigned long*)FLASH_STORE;
		unsigned long* buffer_pmem = (unsigned long*)FLASH_BUFFER;
		
		if(*firmware_pmem == 0xFFFFFFFF)
		{
			printf("No firmware found\r\n");
		} else {
			while(firmware_pmem <= FLASH_BUFFER_END)
			{
				if(*firmware_pmem == 0xFFFFFFFF)
				{
					printf("Firmware contains %d bytes\n\r", buffer_size);
					break;
				}
				buffer_size +=4;
				firmware_pmem++;
			}
		}

		if(*buffer_pmem == 0xFFFFFFFF)
		{
			printf("Upload buffer empty\r\n");
		} else {
			buffer_size = 0;
			while(buffer_pmem <= FLASH_BUFFER_END)
			{
				if(*buffer_pmem == 0xFFFFFFFF)
				{
					printf("Buffer contains %d bytes\n\r", buffer_size);
					break;
				}
				buffer_size +=4;
				buffer_pmem++;
			}
		}
		
		return;
	}
	
	// Display help
	if (strcmp(command, "help") == 0)
	{
		printhelp();
		return;
	}

	if (strcmp(command, "buffer")==0)
	{
		// Display contents of firmware update region (ending @ first 0xFFFFFFFF)
		unsigned long* pmem = (unsigned long*)FLASH_BUFFER;
		while(pmem <= FLASH_BUFFER_END)
		{
			if(*pmem == 0xFFFFFFFF)
			{
				return;
			}
			printf("Addr: %p  Val: 0x%l08x\n\r", (void *)pmem, *pmem);
			pmem++;
		}
		return;
	}
	
	if (strcmp(command, "flash")==0)
	{
		// Display contents of firmware update region (ending @ first 0xFFFFFFFF)
		unsigned long* pmem = (unsigned long*)FLASH_STORE;
		while(pmem <= FLASH_STORE_END)
		{
			if(*pmem == 0xFFFFFFFF)
			{
				return;
			}
			printf("Addr: %p  Val: 0x%l08x\n\r", (void *)pmem, *pmem);
			pmem++;
		}
		return;
	}
	
	// Unknown Command
	printf("Unknown command\r\n");
	return;
}

/*
*	Print the intro screen
*	ASCII art generated from http://patorjk.com/software/taag/
*
*/
void printintro(void)
{
	printf("\r\n");
	printf("Zodiac FX Bootloader %s\r\n", VERSION);
	printf("\r\n\n");
	printf("Type 'help' for a list of available commands\r\n");
	return;
}

/*
*	Print a list of available commands
*
*
*/
void printhelp(void)
{
	printf("\r\n");
	printf("The following commands are currently available:\r\n");
	printf("\r\n");
	printf("upload\r\n");
	printf("update\r\n");
	printf("buffer\r\n");
	printf("flash\r\n");
	printf("status\r\n");
	printf("\r\n");
	return;
}
