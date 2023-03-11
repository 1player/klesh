/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_BIT_H
#define KERNEL_BIT_H

#include <types.h>

/* From Misc/ll_bit.asm */
unsigned int bit_find_set(unsigned int value);
unsigned int bit_find_reset(unsigned int value);
unsigned int bit_set(unsigned int value, unsigned int bit_index);
unsigned int bit_reset(unsigned int value, unsigned int bit_index);
unsigned int bit_invert(unsigned int value, unsigned int bit_index);
unsigned int bit_test(unsigned int value, unsigned int bit_index);

#endif /* !defined KERNEL_BIT_H */
