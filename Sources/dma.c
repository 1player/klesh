/*
 * dma.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-04-06
 *
 */

#include <dma.h>
#include <mm.h>
#include <io.h>
#include <memory.h>

#define DMA_CHANNELS	8

static struct dma_port_info {
	uint16_t command;
	uint16_t address;
	uint16_t count;
	uint16_t page;
	uint16_t mask;
	uint16_t mode;
	uint16_t flip_flop;
} dma_ports[DMA_CHANNELS] = {
	{ 0x08, 0x00, 0x01, 0x87, 0x0A, 0x0B, 0x0C}, 
	{ 0x08, 0x02, 0x03, 0x83, 0x0A, 0x0B, 0x0C}, 
	{ 0x08, 0x04, 0x05, 0x81, 0x0A, 0x0B, 0x0C}, 
	{ 0x08, 0x06, 0x07, 0x82, 0x0A, 0x0B, 0x0C}, 
	{ 0xD0, 0xC0, 0xC1, 0x8F, 0xD4, 0xD6, 0xD8}, // Never used, cascade
	{ 0xD0, 0xC2, 0xC3, 0x8B, 0xD4, 0xD6, 0xD8}, 
	{ 0xD0, 0xC4, 0xC5, 0x89, 0xD4, 0xD6, 0xD8}, 
	{ 0xD0, 0xC6, 0xC7, 0x8A, 0xD4, 0xD6, 0xD8}
};

err_t dma_transfer(unsigned channel, void *dest, size_t len, unsigned read_flag)
{
unsigned char *dma_buffer;
unsigned int addr = (unsigned int)dest;

	// Sanity check
	if ((channel > DMA_CHANNELS - 1) || channel == 4)
		return ERROR_INVALID;

	// Setup and start the transfer
	//memory_copy(dma_buffer, dest, len);
	port_write_byte(dma_ports[channel].mask, channel | 4);			// Unmask channel
	port_write_byte(dma_ports[channel].flip_flop, 0);			// Reset flip flop
	if (read_flag)
		port_write_byte(dma_ports[channel].mode, 0x56);			// Device -> Memory
	else
		port_write_byte(dma_ports[channel].mode, 0x5A);			// Memory -> Device
	port_write_byte(dma_ports[channel].page, addr >> 16);			// Page address
	port_write_byte(dma_ports[channel].address, addr & 0xFF);		// Offset low byte
	port_write_byte(dma_ports[channel].address, (addr >> 8) & 0xFF);		// Offset high byte
	port_write_byte(dma_ports[channel].count, (len - 1) & 0xFF);		// Length low byte
	port_write_byte(dma_ports[channel].count, ((len - 1) >> 8) & 0xFF);	// Length high byte
	port_write_byte(dma_ports[channel].mask, channel);			// Mask and startup

	return 0;
}

err_t dma_init(void)
{
	return 0;
}
