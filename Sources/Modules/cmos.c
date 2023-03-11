/*
 * Drivers/cmos.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-02-07
 *
 */

#include <modules.h>
#include <io.h>
#include <types.h>

#define MAX_CMOS_REG	0x7F

uint8_t cmos_read(uint8_t reg)
{
uint8_t data;

	if (reg > MAX_CMOS_REG)
		return ERROR_INVALID;

	port_write_byte(0x70, reg);
	port_read_byte(0x71, &data);
	
	return data;
}
