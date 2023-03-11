/*
 * console.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-11-23 
 *	2005-05-01: Merged video.c with keyboard functions to create console.c
 *
 */

#include <kernel.h>
#include <console.h>
#include <types.h>
#include <io.h>
#include <memory.h>
#include <keyboard.h>
#include <vararg.h>

static volatile uint16_t *video_fb;
static uint16_t video_index_reg;
static uint16_t video_data_reg;
static int video_width = 80;
static int video_height = 25;
static uint8_t video_foreground = 0x7;
static unsigned int boot_step_length;		// Length of a step
static unsigned int boot_next_step;		// Position of the next step start
static unsigned int boot_steps_left;		// Steps to complete

#define BOOT_BAR_START		2
#define BOOT_BAR_END		(video_width - BOOT_BAR_START - 1)
#define BOOT_BAR_LENGTH		(BOOT_BAR_END - BOOT_BAR_START)
#define BOOT_BAR_TOP		20
#define BOOT_BAR_CENTER		22
#define BOOT_BAR_BOTTOM		23

/********************
 * Cursor functions *
 ********************/

/* Read the video card registers to retrieve the cursor location. */
__init__ static void console_cursor_get(uint16_t *pos)
{
	/* Get the high byte of the position for the video register, position 0x0E */
	port_write_byte(video_index_reg, 0x0E);
	port_read_byte(video_data_reg, (uint8_t *)pos);
	*pos <<= 8;
	
	/* Read the low byte */
	port_write_byte(video_index_reg, 0x0F);
	port_read_byte(video_data_reg, (uint8_t *)pos);
}

/* Set the cursor position on the video card registers */
__init__ static void console_cursor_set(uint16_t pos)
{
	/* Write the high byte to the video register */
	port_write_byte(video_index_reg, 0x0E);
	port_write_byte(video_data_reg, (uint8_t)((pos >> 8) & 0xFF));
	
	/* Write the low one */
	port_write_byte(video_index_reg, 0x0F);
	port_write_byte(video_data_reg, (uint8_t)(pos & 0xFF));
}

/* Show or hide the cursor */
void console_cursor_show(int enable)
{
	port_write_byte(video_index_reg, 0x0A);
	port_write_byte(video_data_reg, enable ? 0x0e : 0x1e);
	port_write_byte(video_index_reg, 0x0B);
	port_write_byte(video_data_reg, 0x0f);
}

/*******************
 * Misc. functions *
 *******************/

__init__ void console_clear(void)
{
int i;
	
	/* Fill in the screen with NULL bytes and set the text 
	   grey on a black foreground */
	memory_fill((void *)video_fb, 0x07200720, video_width * video_height / 2);
	
	/* Reset the video cursor */
	console_cursor_set(0);
	
	/* Reset the video color */
	video_foreground = 0x7;
}

/*************************
 * Video write functions *
 *************************/

static inline void do_write_char(unsigned char c, void *private)
{
uint16_t *pos = (uint16_t *)private;

	switch (c) {
		case '\b':
			(*pos)--;
			video_fb[*pos] = (video_foreground << 8);
			return;

		case '\n':
			*pos += video_width;
			video_foreground = 0x7;
		case '\r':
			*pos -= *pos % video_width;
			break;
		case '\t':
			*pos += 8 - (*pos % 8);
			break;
		
		default:
			video_fb[(*pos)++] = (video_foreground << 8) | c;	/* Grey on black */
	}
	
	/* If the cursor is offscreen, scroll the screen one line up */
	if (*pos >= (video_width * video_height)) {
		/* Scroll the screen */
		memory_copy((void *)&video_fb[0], (void *)&video_fb[video_width], video_width * (video_height - 1) * 2);
		
		/* Clear the last line */
		memory_fill((void *)&video_fb[video_width * (video_height - 1)], 0x07000700, video_width / 2);
		
		/* Set the cursor on the line before */
		*pos -= video_width;
	}
}

err_t console_write_syscall(void *arg)
{
	console_write((unsigned char *)arg);
	
	return 0;
}
	

__init__ void console_write(unsigned char *message)
{
uint16_t pos;
	
	console_cursor_get(&pos);
	
	while (*message)
		do_write_char(*message++, (void *)&pos);
	
	console_cursor_set(pos);
}

void console_write_char(unsigned char c)
{
uint16_t pos;

	console_cursor_get(&pos);
	
	do_write_char(c, (void *)&pos);
	
	console_cursor_set(pos);
}

__init__ size_t console_write_formatted(unsigned char *message, ...)
{
size_t ret;
va_list args;
uint16_t pos;
	
	console_cursor_get(&pos);
	
	va_start(args, message);
	ret = string_format_indirect_ex(do_write_char, (void *)&pos, message, args);
	va_end(args);
	
	console_cursor_set(pos);
	
	return ret;
}

__init__ size_t console_write_formatted_indirect(unsigned char *message, va_list args)
{
size_t ret;
uint16_t pos;
	
	console_cursor_get(&pos);
	
	ret = string_format_indirect_ex(do_write_char, (void *)&pos, message, args);
	
	console_cursor_set(pos);
	
	return ret;
}

inline void console_goto(uint16_t x, uint16_t y)
{
	console_cursor_set(y * video_width + x);
}

inline void console_clear_line(uint16_t y)
{
	memory_fill((void *)&video_fb[video_width * y], 0x07000700, video_width / 2);
}

inline void console_color(uint8_t foreground)
{
	video_foreground = foreground & 0xF;
}

/**************************
 * Console read functions *
 **************************/

unsigned int console_read(unsigned char *buffer, size_t size)
{
unsigned int key;
unsigned int count = 0;

	size--;
	
	while (1) {
		key = keyboard_get_key();
		if (key & 0xFF) {
			if (count < size) {
				key &= 0xFF;
				buffer[count++] = key;
				console_write_char(key);
			}
		} else if (key == KEY_RETURN) {
			console_write_char('\n');
			break;
		} else if (key == KEY_BACKSPACE) {
			if (count) {
				buffer[--count] = 0;
				console_write_char('\b');
			}
		}

			
	}
	buffer[count] = 0;

	return count;		
}

err_t console_read_syscall(void *arg)
{
struct {
	unsigned char *buffer;
	size_t	size;
} *read_syscall_args = arg;

	return console_read(read_syscall_args->buffer, read_syscall_args->size);
}

/***************
 * Boot screen *
 ***************/

static void console_boot_write_center(uint16_t y, unsigned char *message)
{
	console_goto( ((video_width - string_length(message)) / 2) - 1, y);
	console_clear_line(y);
	console_write(message);
}

void console_boot_step(unsigned char *message)
{
#if 0
	console_color(8);	
	console_goto(boot_next_step, BOOT_BAR_CENTER);

	if (boot_steps_left-- == 1) {
		unsigned int i = boot_next_step;
		while (i++ < BOOT_BAR_END)
			console_write_char(0xfe);
	} else {
		unsigned int i = boot_step_length;
		while (i--)
			console_write_char(0xfe);
	}

	boot_next_step += boot_step_length;

	console_color(8);
	console_boot_write_center(BOOT_BAR_BOTTOM, message);
#endif
}

void console_boot_init(unsigned int steps)
{
unsigned int i;

	/* Clean up the screen */
	console_clear();

	/* Turn off the cursor */
	console_cursor_show(0);

	/* Initialize the vars */
	boot_step_length = BOOT_BAR_LENGTH / steps;
	boot_next_step = BOOT_BAR_START + 1;
	boot_steps_left = steps;

	/* Write the decorations */
	console_goto(BOOT_BAR_START, BOOT_BAR_CENTER);
	console_color(9);
	console_write_char('[');

	console_color(8);
	for (i = boot_next_step; i < BOOT_BAR_END; i++)
		console_write_char(0xf9);

	console_goto(BOOT_BAR_END, BOOT_BAR_CENTER);
	console_color(9);
	console_write_char(']');

	/* Write the text */
	console_color(9);
	console_boot_write_center(BOOT_BAR_TOP, "Initializing Klesh...");
}

/**************************
 * Console initialization *
 **************************/
	

__init__ int console_init(void)
{
uint8_t video_status;

	port_write_byte(0x3d4, 0x0A);
	port_read_byte(0x3d4, &video_status);
	
	/* Check whether we have a color video card */
	port_read_byte(0x3cc, &video_status);
	if (video_status & 1) {
		video_fb = (uint16_t *)0xb8000;
		video_index_reg = 0x3d4;
		video_data_reg = 0x3d5;
	}
	else {
		video_fb = (uint16_t *)0xb0000;
		video_index_reg = 0x3b4;
		video_data_reg = 0x3b5;
		
		console_write("Klesh needs a color video card to run correctly.\n");
		return 1;
	}	

	return 0;
}
