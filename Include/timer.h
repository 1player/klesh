/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <types.h>

#define TIMER_8253_PORT_CONTROL		0x43
#define TIMER_8253_PORT_COUNTER0	0x40
#define TIMER_8253_PORT_COUNTER1	0x41
#define TIMER_8253_PORT_COUNTER2	0x42

/* Frequencies (in Hz) */
#define TIMER_GRANULARITY_HZ	1000
#define TIMER_GRANULARITY_MS	(1000 / TIMER_GRANULARITY_HZ)

// System ticks, incremented in x86.asm - _irq0_handler
volatile unsigned int _ticks;

err_t timer_init(void);

#endif /* !defined KERNEL_TIMER_H */
