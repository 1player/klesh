/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include <types.h>
#include <vararg.h>

size_t string_format_indirect(unsigned char *buffer, unsigned char *format, va_list arguments);
size_t string_format_indirect_ex(void (*emit_func)(unsigned char c, void *private), void *private,
					unsigned char *format, va_list arguments);
size_t string_format(unsigned char *buffer, unsigned char *format, ...);
size_t string_format_ex(void (*emit_func)(unsigned char c, void *private), void *private, 
				unsigned char *format, ...);
size_t string_length(unsigned char *string);
int string_compare(unsigned char *str1, unsigned char *str2);

#endif /* !defined KERNEL_STRING_H */
