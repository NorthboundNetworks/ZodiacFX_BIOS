/**
 * @file
 * config_zodiac.h
 *
 * This file contains the configuration for the Zodiac FX
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

#ifndef CONFIG_BOOTLOADER_H_
#define CONFIG_BOOTLOADER_H_


#define VERSION "1.10"		// Firmware version number

#define SHARED_BUFFER_LEN 2048
#define FLASH_BUFFER 0x448000
#define FLASH_STORE 0x410000
#define FLASH_BUFFER_END 0x480000
#define FLASH_STORE_END 0x448000

#endif /* CONFIG_BOOTLOADER_H_ */
