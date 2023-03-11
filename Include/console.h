/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include <types.h>
#include <vararg.h>

int console_init(void);

void console_write(unsigned char *message);
void console_write_char(unsigned char c);
size_t console_write_formatted(unsigned char *message, ...);
size_t console_write_formatted_indirect(unsigned char *message, va_list args);

unsigned int console_read(unsigned char *buffer, size_t size);

inline void console_goto(uint16_t x, uint16_t y);
inline void console_clear_line(uint16_t y);
inline void console_color(uint8_t foreground);
void console_clear(void);

void console_cursor_show(int enable);

void console_boot_init(unsigned int steps);
void console_boot_step(unsigned char *message);
static void console_boot_write_center(uint16_t y, unsigned char *message);


err_t console_write_syscall(void *arg);
err_t console_read_syscall(void *arg);

#endif /* !defined KERNEL_CONSOLE_H */
