/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <types.h>

#define MM_AREA_KERNEL_START		0x00000000
#define MM_AREA_KERNEL_END		0x50000000

#define MM_AREA_USER_START		MM_AREA_KERNEL_END
#define MM_AREA_USER_CODEDATA_START	0xC0000000
#define MM_AREA_USER_CODEDATA_END	0xE0000000
#define MM_AREA_USER_END		0xFFFFF000

/* init.c */
err_t mm_init(void);

/* ppage.c */
err_t mm_ppage_init(uint32_t *kernel_end);
err_t mm_ppage_pop(uint32_t *ptr, size_t count);
void mm_ppage_push(uint32_t *ptr, size_t count);
unsigned int mm_ppage_get_free(void);
unsigned int mm_ppage_get_total(void);

/* heap.c */
void* mm_heap_allocate(size_t bytes);
void* mm_heap_allocate_aligned(size_t alignment, size_t bytes);
void* mm_heap_reallocate(void* oldmem, size_t bytes);
void mm_heap_free(void* mem);

/* map.c */
err_t mm_map(uint32_t start, uint32_t length, unsigned flags);
err_t mm_map_physical(uint32_t virtual, uint32_t physical, uint32_t length, unsigned flags);
void mm_unmap(uint32_t start, uint32_t length);
void mm_unmap_physical(uint32_t start, uint32_t length);
err_t mm_map_check(uint32_t page);
err_t mm_map_page_directory(uint32_t page_dir);

/* dma.c */
err_t mm_dma_init(void);
err_t mm_dma_allocate(size_t len, void **buffer);

#endif /* !defined KERNEL_MM_H */
