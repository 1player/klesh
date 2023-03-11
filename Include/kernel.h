/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_KERNEL_H
#define KERNEL_KERNEL_H

#include <console.h>
#include <string.h>
#include <types.h>

#define return_on_failure(func)		{			\
					err_t ret = func;	\
					if (ret)		\
						return ret;	\
					}

#ifdef DEBUG

	#define __init__
	#define assert(x)	
			
#else	/* defined DEBUG */

	#define __init__ __attribute__((section(".init")))

	#define assert(x)	

#endif

#define kernel_panic(message, ...)	_kernel_panic(__func__, __FILE__, __LINE__, message, ## __VA_ARGS__)
#define kernel_bug(message, ...)	_kernel_bug(__func__, __FILE__, __LINE__, message, ## __VA_ARGS__)

extern void _kernel_panic(const unsigned char *function, const unsigned char *file, const unsigned int line, unsigned char *message, ...);
extern void _kernel_bug(const unsigned char *function, const unsigned char *file, const unsigned int line, unsigned char *message, ...);

#endif /* !defined KERNEL_KERNEL_H */
