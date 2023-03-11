/*
 * timer.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-12-20
 *
 */

#include <timer.h>
#include <io.h>

err_t timer_init(void)
{
uint16_t value;

	// Initialize system ticks
	_ticks = 0;

	/*
	 * Initialize the Programmable Interval Timer (8253/8254)
	 */

	/* Use the first counter for the scheduler */
	value = 1193181 / TIMER_GRANULARITY_HZ;	

	port_write_byte(TIMER_8253_PORT_CONTROL, (3 << 1) | (3 << 4));	/* Square wave generator, load first LSB then MSB, counter 0 */
	port_write_byte(TIMER_8253_PORT_COUNTER0, value & 0xFF);
	port_write_byte(TIMER_8253_PORT_COUNTER0, (value >> 8) & 0xFF);

	return 0;
}
