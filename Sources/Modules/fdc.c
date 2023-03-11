/*
 * Drivers/fdc.c
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
#include <string.h>
#include <console.h>
#include <memory.h>
#include <io.h>
#include <interrupt.h>
#include <process.h>
#include <cpu.h>
#include <dma.h>
#include <mm.h>

// Generic
#define FLOPPY_BLOCK_SIZE		512
#define FLOPPY_MAX_DEVICES		2

// Floppy ports
#define FDC_PORT_DOR			0x3F2
#define FDC_PORT_MSR			0x3F4
#define FDC_PORT_DATA			0x3F5
#define FDC_PORT_DRS			0x3F7

// Floppy commands
#define FDC_COMMAND_SENSE_INTERRUPT	0x08
#define FDC_COMMAND_SEEK		0x0F
#define FDC_COMMAND_READ_SECTOR		0x66		// Read and skip deleted data

/* From Drivers/cmos.c */
extern uint8_t cmos_read(uint8_t reg);

static struct floppy_type {
	unsigned char 	*name;
	unsigned int	size_in_blocks;
} floppy_types[] = {
	{ "",	0 },
	{ "360KB 5.25\"", 0 },
	{ "1.2MB 5.25\"", 0 },
	{ "720KB 3.5\"", 0 },
	{ "1.44MB 3.5\"", 2880 },
	{ "2.88MB 3.5\"", 0 },
};
static unsigned		found_drives = 0;
static unsigned char	*dma_buffer;

// Flags
static unsigned		fdc_interrupt_flag = 0;			// A FDC interrupt has been received
static unsigned		fdc_interrupt_wait_flag = 0;		// Are we waiting for an interrupt?

// Prototypes
static err_t fdc_send(unsigned count, ...);
static err_t fdc_get(unsigned count, ...);

/*******************************
 * Floppy interrupt management *
 *******************************/

static void fdc_isr(void)
{
	if (!fdc_interrupt_wait_flag) {
		// Unexpected interrupt. Force a controller reset
		// TODO: handle this correctly
		console_write("Unexpected interrupt. Panic.");
		while (1);
	}

	fdc_interrupt_flag = 1;
}

static err_t fdc_interrupt_wait(unsigned int timeout, unsigned sense)
{
uint8_t sr0 = 50, track = 60;
unsigned int count;

	// Sanity check
	if (!fdc_interrupt_wait_flag) {
		console_write("FDC: Called interrupt wait with interrupt wait flag reset!\n");
		while (1);
	}

	// Check for interrupt each ms, no longer than 1000 ms
	while (timeout--) {
		if (fdc_interrupt_flag)
			goto got_interrupt;
	
		process_thread_sleep_time(1);
	}

	return ERROR_TIMEOUT;

got_interrupt:

	fdc_interrupt_wait_flag = 0;
	fdc_interrupt_flag = 0;

	// Send a sense interrupt command if needed
	if (sense) {
		fdc_send(1, FDC_COMMAND_SENSE_INTERRUPT);
		fdc_get(2, &sr0, &track);
	}

	return 0;
	
}

static err_t fdc_send(unsigned count, ...)
{
va_list args;
uint8_t msr;
unsigned int timeout;

	va_start(args, count);
	
	process_schedule_disable();

	while (count--) {
		// Timed check for RQM == 1 and DIO == 0 in MSR
		timeout = 100;
		while (timeout--) {
			port_read_byte(FDC_PORT_MSR, &msr);

			if ((msr & 0xC0) == 0x80)
				goto got_interrupt;

			process_thread_sleep_time(1);
		}

		va_end(args);

		return ERROR_TIMEOUT;

got_interrupt:

		port_write_byte(FDC_PORT_DATA, va_arg(args, uint8_t));
	}
	
	process_schedule_enable();

	va_end(args);

	return 0;
}

static err_t fdc_get(unsigned count, ...)
{
va_list args;
uint8_t msr;
unsigned int timeout;

	va_start(args, count);
	
	process_schedule_disable();

	while (count--) {
		timeout = 100;
		while (timeout--) {
			port_read_byte(FDC_PORT_MSR, &msr);

			if ((msr & 0xD0) == 0xD0)
				goto got_interrupt;

			process_thread_sleep_time(1);
		}

		va_end(args);

		return ERROR_TIMEOUT;

got_interrupt:

		port_read_byte(FDC_PORT_DATA, va_arg(args, uint8_t *));
	}
	
	process_schedule_enable();

	va_end(args);

	return 0;
}

static err_t fdc_reset(void)
{
	fdc_interrupt_wait_flag = 1;
	
	// Stop the motor and disable IRQ/DMA
	port_write_byte(FDC_PORT_DOR, 0);

	// Program data rate to 500 Kb/S
	port_write_byte(FDC_PORT_DRS, 0);

	// Re-enable IRQ/DMA
	port_write_byte(FDC_PORT_DOR, 0x0C);

	// Wait for interrupt
	if (fdc_interrupt_wait(1000, 1)) {
		console_write("FDC: floppy has timed out during reset... aborting\n");
		return ERROR_TIMEOUT;
	}
	

	return 0;
}

// Spin up/down motor of the selected drive
static void fdc_motor_control(unsigned drive, unsigned enable)
{
uint8_t dor_value;

	port_read_byte(FDC_PORT_DOR, &dor_value);
	
	if (enable)
		dor_value |= 0x10 << drive;
	else
		dor_value &= ~(0x10 << drive);
	
	port_write_byte(FDC_PORT_DOR, dor_value);
}

static err_t fdc_seek(unsigned drive, uint8_t track)
{
	// TODO: make sure that the motor is on

	// Seek
	fdc_interrupt_wait_flag = 1;
	fdc_send(3, FDC_COMMAND_SEEK, 0, track);

	// Wait for interrupt
	if (fdc_interrupt_wait(1000, 1)) {
		console_write("FDC: floppy has timed out during seek... aborting\n");
		return ERROR_TIMEOUT;
	}

	return 0;
}
	

err_t fdc_read(void *buffer, unsigned int lba, size_t sector_count)
{
uint8_t head, cylinder, sector, st0, st1, st2, n;
int i;
unsigned int cur_sector;

	fdc_motor_control(0, 1);

	cur_sector = lba;

	while (sector_count--) {
		// Calculate CHS from LBA
		head = (cur_sector % (18 * 2)) / (18);
	  	cylinder = cur_sector / (18 * 2);
		sector = cur_sector % 18 + 1;

		// Seek to the right track
		fdc_seek(0, cylinder);

		// Setup DMA and interrupts
		dma_transfer(2, dma_buffer, FLOPPY_BLOCK_SIZE, 1);	
		fdc_interrupt_wait_flag = 1;

		// Send command
		fdc_send(9, FDC_COMMAND_READ_SECTOR, head << 2, cylinder, head, sector, 2, 18, 0x1B, 0xFF);

		// Wait for completion
		if (fdc_interrupt_wait(1000, 0)) {
			console_write("FDC: floppy has timed out during reading... aborting\n");
			return ERROR_TIMEOUT;
		}

		// Read status
		fdc_get(7, &st0, &st1, &st2, &cylinder, &head, &sector, &n);

		memory_copy(buffer, dma_buffer, FLOPPY_BLOCK_SIZE);

		buffer += FLOPPY_BLOCK_SIZE;
		cur_sector++;
	}

	fdc_motor_control(0, 0);

	return 0;
}

/******************
 * Initialization *
 ******************/

static int fdc_add_drive(unsigned char cmos_type)
{
	/* Check if the floppy type is supported */

	if (!cmos_type)
		return 1;

	if ( cmos_type > (sizeof(floppy_types) / sizeof(struct floppy_type)) ) {
		console_write("FDC: Found a unknown floppy drive but its size is not supported yet\n");
		return 1;
	}

	if (!floppy_types[cmos_type].size_in_blocks) {
		console_write_formatted("FDC: Found a %s floppy drive but its size is not supported yet\n", floppy_types[cmos_type].name);
		return 1;
	}

//	console_write_formatted("FDC: Found a %s floppy drive!\n", 
//			floppy_types[cmos_type].name);

	found_drives++;

	return 0;
}

err_t fdc_init(void)
{
uint8_t fd_types;
err_t ret;
void *buffer;

	/* Read on the CMOS the number of floppy drives */
	fd_types = cmos_read(0x10);

	if (!fd_types) {
		console_write("FDC: No floppy drives found!\n");
		return 0;
	}

	fdc_add_drive((fd_types >> 4) & 0x7);
	fdc_add_drive(fd_types & 0x7);

	// Register FDC IRQ
	interrupt_irq_register(6, fdc_isr);
	interrupt_irq_enable(6);

	// Allocate a DMA buffer
	mm_dma_allocate(FLOPPY_BLOCK_SIZE, (void **)&dma_buffer);

	// Reset drive
	ret = fdc_reset();
	if (ret)
		return ret;

	//fdc_read(2, 1);

	return 0;
}
