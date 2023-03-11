/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_INTERRUPT_H
#define KERNEL_INTERRUPT_H

#include <types.h>

/* 8259 PIC ports */
#define INTERRUPT_8259_MASTER_CMD	0x20
#define INTERRUPT_8259_MASTER_DATA	0x21
#define INTERRUPT_8259_SLAVE_CMD	0xA0
#define INTERRUPT_8259_SLAVE_DATA	0xA1

/* From x86.asm */
extern void interrupt_enable(void);
extern void interrupt_disable(void);

err_t interrupt_init(void);
err_t interrupt_init_doublefault(void);

err_t interrupt_irq_register(uint8_t number, void (*isr)(void));
err_t interrupt_irq_enable(uint8_t number);
err_t interrupt_irq_disable(uint8_t number);

#endif /* !defined KERNEL_INTERRUPT_H */
