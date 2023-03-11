/*
 * Drivers/ata.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-01-16
 *
 */


#include <modules.h>
#include <types.h>
#include <console.h>
#include <io.h>
#include <cpu.h>
#include <interrupt.h>

/* Status flags */
#define ATA_STATUS_FLAG_ERROR		0x01
#define ATA_STATUS_FLAG_INDEX		0x02
#define ATA_STATUS_FLAG_EC_DATA		0x04	/* Error corrected data */
#define ATA_STATUS_FLAG_DATA_REQUEST	0x08
#define ATA_STATUS_FLAG_SEEK_COMPLETE	0x10
#define ATA_STATUS_FLAG_FAULT		0x20
#define ATA_STATUS_FLAG_READY		0x40
#define ATA_STATUS_FLAG_BUSY		0x80

/* Controller port indexes */
#define ATA_PORT_INDEX_DATA 			0x00	/* Read/Write */
#define ATA_PORT_INDEX_ERROR			0x01 	/* Read */
#define ATA_PORT_INDEX_PRECOMP			0x01 	/* Write */
#define ATA_PORT_INDEX_SECTOR_COUNT		0x02 	/* Read/Write */
#define ATA_PORT_INDEX_SECTOR_NUMBER		0x03 	/* Read/Write */
#define ATA_PORT_INDEX_CYLINDER_LOW		0x04 	/* Read/Write */
#define ATA_PORT_INDEX_CYLINDER_HIGH		0x05 	/* Read/Write */
#define ATA_PORT_INDEX_HEAD			0x06 	/* Read/Write */
#define ATA_PORT_INDEX_STATUS			0x07 	/* Read */
#define ATA_PORT_INDEX_COMMAND			0x07 	/* Write */
#define ATA_PORT_INDEX_CONTROL			0x206 	/* Write */

static unsigned int num_ata_devices = 0, num_atapi_devices = 0;


static int ata_check_status(uint16_t base, uint8_t set_bits, uint8_t reset_bits)
{
uint8_t status;

	port_read_byte(base + ATA_PORT_INDEX_STATUS, &status);
	
	if ( ((status & set_bits) ^ set_bits) != 0)
		return 0;
		
	if ( (status & reset_bits) == 0 )
		return 1;
		
	return 0;
}

static void detect_device(uint16_t base, int slave)
{
unsigned int timeout;
uint8_t sc, sn, cl, ch, status;

	/* Select the device */
	port_write_byte(base + ATA_PORT_INDEX_CONTROL, 0x08);
	port_write_byte(base + ATA_PORT_INDEX_HEAD, slave ? 0xB0 : 0xA0);
	delay(1);
	
	/* See if the device is present */
	port_write_byte(base + ATA_PORT_INDEX_HEAD, slave ? 0xB0 : 0xA0);
	delay(1);
	port_write_byte(base + ATA_PORT_INDEX_SECTOR_COUNT,  0x55);
	port_write_byte(base + ATA_PORT_INDEX_SECTOR_NUMBER, 0xAA);
	port_write_byte(base + ATA_PORT_INDEX_SECTOR_COUNT,  0xAA);
	port_write_byte(base + ATA_PORT_INDEX_SECTOR_NUMBER, 0x55);
	port_write_byte(base + ATA_PORT_INDEX_SECTOR_COUNT,  0x55);
	port_write_byte(base + ATA_PORT_INDEX_SECTOR_NUMBER, 0xAA);
	port_read_byte(base + ATA_PORT_INDEX_SECTOR_COUNT, &sc);
	port_read_byte(base + ATA_PORT_INDEX_SECTOR_NUMBER, &sn);
	
	if ( (sc != 0x55) && (sn != 0xAA) )
		return;
	
	/* Reset the device (this selects Device 0) */
	port_write_byte(base + ATA_PORT_INDEX_HEAD, slave ? 0xB0 : 0xA0);
	delay(1);
	port_write_byte(base + ATA_PORT_INDEX_CONTROL, 0x0C);
	delay(1);
	port_write_byte(base + ATA_PORT_INDEX_CONTROL, 0x08);
	delay(1);
	
	/* Wait device to be ready (wait no longer than 35 s) */
	if (slave) {
		port_write_byte(base + ATA_PORT_INDEX_HEAD, 0xB0);
		delay(1);
	}
	
	timeout = 0;
	do {	
		port_read_byte(base + ATA_PORT_INDEX_SECTOR_COUNT, &sc);
		port_read_byte(base + ATA_PORT_INDEX_SECTOR_NUMBER, &sn);
	
		if ( (sc == 1) && (sn == 1) ) {
			if (ata_check_status(base, 0, ATA_STATUS_FLAG_BUSY))
				goto ready;
		}
			
			
		delay(1);
	} while (timeout++ < 35000);

	return;
	
ready:	

	/* Identify not present devices from ATA or ATAPI drives */
	port_write_byte(base + ATA_PORT_INDEX_HEAD, slave ? 0xB0 : 0xA0);
	delay(1);
	port_read_byte(base + ATA_PORT_INDEX_SECTOR_COUNT, &sc);
	port_read_byte(base + ATA_PORT_INDEX_SECTOR_NUMBER, &sn);
	if ( (sc == 1) && (sn == 1) ) {
		port_read_byte(base + ATA_PORT_INDEX_CYLINDER_LOW, &cl);
		port_read_byte(base + ATA_PORT_INDEX_CYLINDER_HIGH, &ch);
		port_read_byte(base + ATA_PORT_INDEX_STATUS, &status);
		
		if ( (cl == 0x00) && (ch == 0x00) && (status != 0x00) ) {
			/* Found an ATA device */
			num_ata_devices++;
		} else if ( (cl == 0x14) && (ch == 0xEB) ) {
			/* TODO: support for ATAPI devices */
			num_atapi_devices++;
		}
	}
	
	return;
}

/**
 * Initialize the ATA driver
 */
err_t ata_init(void)
{
char buf[64];

#if 0
	interrupt_irq_register(14, ata_irq);
	interrupt_irq_register(15, ata_irq);
	
	interrupt_irq_enable(14);
	interrupt_irq_enable(15);
#endif
	
//	console_write("ATA: finding devices... ");

	detect_device(0x1F0, 0);
	detect_device(0x1F0, 1);
	detect_device(0x170, 0);
	detect_device(0x170, 1);

	//console_write_formatted("%u ATA and %u ATAPI devices found!\n", num_ata_devices, num_atapi_devices);

	return 0;
}
