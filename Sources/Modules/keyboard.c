/*
 * Drivers/keyboard.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-04-21
 *
 */

#include <modules.h>
#include <interrupt.h>
#include <console.h>
#include <kernel.h>
#include <io.h>
#include <keyboard.h>
#include <process.h>

// Keyboard controller commands
#define KEYBOARD_CCOMMAND_SELFTEST	0xAA
#define KEYBOARD_CCOMMAND_IFTEST	0xAB
#define KEYBOARD_CCOMMAND_ENABLE	0xAE

// Keyboard commands (sent throw keyboard data port)
#define KEYBOARD_COMMAND_ENABLE		0xF4
#define KEYBOARD_COMMAND_RESET		0xFF

// Keyboard replies
#define KEYBOARD_REPLY_ACK		0xFA
#define KEYBOARD_REPLY_RESEND		0xFE
#define KEYBOARD_REPLY_POR		0xAA

// Keyboard ports
#define	KEYBOARD_PORT_CONTROLLER	0x64
#define KEYBOARD_PORT_DATA		0x60

extern struct process *kernel_process;

const unsigned int scancode2key[128] = {
	/* 0x00 - 0x0F */
	0, KEY_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', 0x8d, KEY_BACKSPACE, '\t',

	/* 0x10 - 0x1F */
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0x8a, '+', KEY_RETURN, KEY_CTRL, 'a', 's', 

	/* 0x20 - 0x2F */
	'd', 'f', 'g', 'h', 'j', 'k', 'l', 0x95, 0x85, '\\', KEY_SHIFT, 0x97, 'z', 'x', 'c', 'v', 

	/* 0x30 - 0x3F */
	'b', 'n', 'm', ',', '.', '-', KEY_SHIFT, '*', KEY_ALT, ' ', KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
	
};

const unsigned int scancode2key_shift[128] = {
	/* 0x00 - 0x0F */
	0, KEY_ESCAPE, '!', '\"', 0x9c, '$', '%', '&', '/', '(', ')', '=', '?', '^', KEY_BACKSPACE, '\t',

	/* 0x10 - 0x1F */
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0x82, '*', KEY_RETURN, KEY_CTRL, 'A', 'S', 

	/* 0x20 - 0x2F */
	'D', 'F', 'G', 'H', 'J', 'K', 'L', 0x87, 0xf8, '|', KEY_SHIFT, 0, 'Z', 'X', 'C', 'V', 

	/* 0x30 - 0x3F */
	'B', 'N', 'M', ';', ':', '_', KEY_SHIFT, '*', KEY_ALT, ' ', KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
	
};

unsigned int last_key, a;

sleep_object_t wait_key_sleep = INITIALIZED_SLEEP_OBJECT;

static void keyboard_isr(void)
{
uint8_t data;
unsigned int key;
int key_released;

// Modifiers
static int mod_shift = 0;
static int mod_ctrl = 0;
static int mod_alt = 0;

	port_read_byte(KEYBOARD_PORT_DATA, &data);

	key = mod_shift ? scancode2key_shift[data & 0x7F] : scancode2key[data & 0x7F];
	key_released = data & 0x80;

	// Modifier key selected
	if (key & KEY_MOD_FLAG) {

		switch(key) {
		case KEY_SHIFT:
			mod_shift = !key_released;
			break;

		case KEY_CTRL:
			mod_ctrl = !key_released;
			break;

		case KEY_ALT:
			mod_alt = !key_released;
			break;
			
		}
			
	} else {
		if (!key_released) {
			last_key = key;

			// Wake up all thread waiting for a key
			process_thread_wakeup_object(&wait_key_sleep);
		}
				
	}
}

unsigned int keyboard_get_key(void)
{
	process_thread_sleep_object(&wait_key_sleep);

	return last_key;
}

// Wait for keyboard input buffer (host->kbd) not to be full
static err_t keyboard_wait_input(void)
{
uint8_t status;
unsigned int timeout;

	timeout = 100;
	while (timeout--) {
		port_read_byte(KEYBOARD_PORT_CONTROLLER, &status);
		
		if (!(status & 0x02))
			goto gotcha;
	
		process_thread_sleep_time(1);
	}

	return ERROR_TIMEOUT;

gotcha:
	return 0;
}

// Wait for keyboard output buffer (kbd->host) not to be empty
// Opposite keyboard_wait_input behavior
static err_t keyboard_wait_output(void)
{
uint8_t status;
unsigned int timeout;

	timeout = 100;
	while (timeout--) {
		port_read_byte(KEYBOARD_PORT_CONTROLLER, &status);
		
		if (status & 0x01)
			goto gotcha;
	
		process_thread_sleep_time(1);
	}

	return ERROR_TIMEOUT;

gotcha:
	return 0;
}

static err_t keyboard_write_controller(uint8_t data)
{
	if (keyboard_wait_input())
		return ERROR_TIMEOUT;

	port_write_byte(KEYBOARD_PORT_CONTROLLER, data);

	return 0;
}

static err_t keyboard_write(uint8_t data)
{
	if (keyboard_wait_input())
		return ERROR_TIMEOUT;

	port_write_byte(KEYBOARD_PORT_DATA, data);

	return 0;
}

static err_t keyboard_read(uint8_t *ret)
{
uint8_t status;

	if (keyboard_wait_output())
		return ERROR_TIMEOUT;

	port_read_byte(KEYBOARD_PORT_DATA, ret);

	return 0;
}


static err_t keyboard_reset(void)
{
uint8_t data;
err_t ret;

	keyboard_write_controller(0xAD);

#if 0

	// Clear the output buffer
	do {
		ret = keyboard_read(&data);
	} while (ret != ERROR_TIMEOUT);	
#endif

	// Reset the keyboard
	keyboard_write(0xF5);
	ret = keyboard_read(&data);
	if (ret || data != KEYBOARD_REPLY_ACK) {
		console_write_formatted("KBD: Unable to reset the keyboard (no ACK received) %x %x\n", ret, data);
		return ERROR_NOT_AVAILABLE;
	}
	

	// Set scan code set 2
	keyboard_write(0xF0);
	ret = keyboard_read(&data);
	if (ret || data != KEYBOARD_REPLY_ACK) {
		console_write_formatted("KBD: Unable to set the scan code mode3 %x %x\n", ret, data);
		return ERROR_NOT_AVAILABLE;
	}
	keyboard_write(2);
	ret = keyboard_read(&data);
	if (ret || data != KEYBOARD_REPLY_ACK) {
		console_write("KBD: Unable to set the scan code mode4\n");
		return ERROR_NOT_AVAILABLE;
	}

	keyboard_write(0xFA);

	keyboard_write(0xF4);
	keyboard_read(&data);
	if (data != KEYBOARD_REPLY_ACK) {
		console_write("KBD: Unable to enable keyboard\n");
		return ERROR_NOT_AVAILABLE;
	}

	return 0;
}

err_t keyboard_init(void)
{
	// Keyboard reset
	//return_on_failure(keyboard_reset());

	// Register keyboard IRQ
	interrupt_irq_register(1, keyboard_isr);	
	interrupt_irq_enable(1);

	return 0;
}
