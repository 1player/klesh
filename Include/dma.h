/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_DMA_H
#define KERNEL_DMA_H

#include <types.h>

#define DMA_PAGE_SIZE			65536
#define DMA_NEXT_BOUNDARY(start)	(((start) & ~(DMA_PAGE_SIZE - 1)) + DMA_PAGE_SIZE)

err_t dma_transfer(unsigned channel, void *dest, size_t len, unsigned read_flag);
err_t dma_init(void);

#endif /* !defined KERNEL_DMA_H */
