/*
 * syscalls.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-06-01
 *
 */
 
#include <kernel.h>
#include <console.h>
 
typedef err_t (*syscall)(void *arg);

static syscall misc_table[] = {
	console_write_syscall,		// 0
	console_read_syscall,		// 1
};
const unsigned int misc_table_count	= (sizeof(misc_table) / sizeof(syscall));
 
err_t syscall_misc(unsigned int number, void *arg)
{
	if (number > misc_table_count) 
		kernel_bug("The kernel should terminate on invalid syscall number");
		
	return misc_table[number](arg);
}
