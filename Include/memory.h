/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <types.h>

/* From Misc/memory.asm */
void memory_fill(void *dest, unsigned int pattern, size_t count) __attribute__((nonnull (1)));
void memory_clear(void *dest, size_t count) __attribute__((nonnull (1)));
void memory_copy(void *dest, void *src, size_t count) __attribute__((nonnull (1, 2)));
void memory_swap_word(uint16_t *word);
unsigned int memory_bit_find_set(unsigned int value);
unsigned int memory_bit_find_reset(unsigned int value);
unsigned int memory_bit_set(unsigned int value, unsigned int bit_index);
unsigned int memory_bit_reset(unsigned int value, unsigned int bit_index);
unsigned int memory_bit_invert(unsigned int value, unsigned int bit_index);
unsigned int memory_bit_test(unsigned int value, unsigned int bit_index);

/* From Misc/memory.c */
void memory_sort(void *base, size_t count, size_t element_size, int (*compare_func)(const void *, const void *)) 
	__attribute__((nonnull (1, 4)));

#endif /* !defined KERNEL_MEMORY_H */
