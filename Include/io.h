/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <types.h>

void port_read_byte(uint16_t port, uint8_t *value);
void port_write_byte(uint16_t port, uint8_t value);

void port_read_word(uint16_t port, uint16_t *value);

#endif /* !defined KERNEL_IO_H */
