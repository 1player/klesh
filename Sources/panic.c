/*
 * panic.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-05-15
 *
 */
 
#include <console.h>
#include <interrupt.h>
#include <vararg.h>

extern void _freeze(void);

// Don't call this directly, use the macro 'kernel_panic' instead
void _kernel_panic(unsigned char *function, unsigned char *file, unsigned int line, unsigned char *message, ...)
{
va_list args;

	interrupt_disable();

	console_clear();
	
	va_start(args, message);
	
	console_write_formatted("Kernel panic at function %s (%s:%u):\n\n\t", function, file, line);
	console_write_formatted_indirect(message, args);
	
	va_end(args);
	
	_freeze();
}

// Don't call this directly, use the macro 'kernel_bug' instead
void _kernel_bug(unsigned char *function, unsigned char *file, unsigned int line, unsigned char *message, ...)
{
va_list args;

	interrupt_disable();

	console_clear();
	
	va_start(args, message);
	
	console_write_formatted("Kernel bug at function %s (%s:%u):\n\n\t", function, file, line);
	console_write_formatted_indirect(message, args);
	console_write("\n\nReport entirely this message to neuromancer@paranoici.org\nSorry about that!");
	
	va_end(args);
	
	_freeze();
}
