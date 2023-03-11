/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include <types.h>

/* Special keys */
#define KEY_ESCAPE		0x01000000
#define KEY_BACKSPACE		0x02000000
#define KEY_RETURN		0x03000000
#define KEY_F1			0x04000000
#define KEY_F2			0x05000000
#define KEY_F3			0x06000000
#define KEY_F4			0x07000000
#define KEY_F5			0x08000000

/* Modifier special keys */
#define KEY_MOD_FLAG		0x80000000	// Used to check if a key is a modifier
#define KEY_CTRL		0x81000000
#define KEY_SHIFT		0x82000000
#define KEY_ALT			0x83000000
#define KEY_CAPS		0x84000000

unsigned int keyboard_get_key(void);

#endif /* !defined KERNEL_KEYBOARD_H */
