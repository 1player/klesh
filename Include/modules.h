/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_MODULES_H
#define KERNEL_MODULES_H

#include <types.h>

err_t ata_init(void);

err_t fdc_init(void);
err_t fdc_read(void *buffer, unsigned int lba, size_t sector_count);

err_t keyboard_init(void);
unsigned int keyboard_get_key(void);

err_t ext2_init(void);
err_t ext2_open(unsigned char *path, void **handle);
err_t ext2_file_size(void *handle, unsigned int *size);
err_t ext2_read(void *handle, unsigned char *buffer, size_t from, size_t count);
err_t ext2_close(void *handle);

#endif /* !defined KERNEL_MODULES_H */
