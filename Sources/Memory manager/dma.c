/*
 * Memory manager/dma.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-04-04
 */

#include <mm.h>
#include <multiboot.h>
#include <memory.h>
#include <dma.h>

static struct dma_memory_entry {
	unsigned int 		start;
	unsigned int 		length;

	struct dma_memory_entry	*prev;
	struct dma_memory_entry	*next;
} *dma_memory_pool;

unsigned int dma_entries = 0;

/*
 * Entry management
 */
static err_t mm_dma_entry_add(unsigned int start, size_t length)
{
struct dma_memory_entry *entry;

	entry = (struct dma_memory_entry *)mm_heap_allocate(sizeof(struct dma_memory_entry));
	if (!entry)
		return ERROR_NO_MEMORY;

	entry->start = start;
	entry->length = length;
	entry->prev = 0;
	entry->next = dma_memory_pool;
	if (dma_memory_pool)
		dma_memory_pool->prev = entry;

	dma_memory_pool = entry;

	return 0;
}

static err_t mm_dma_entry_remove(struct dma_memory_entry *ptr)
{
struct dma_memory_entry *prev, *next;

	prev = ptr->prev;
	next = ptr->next;

	// Restore old settings
	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;

	// Free
	mm_heap_free(ptr);	
	
	return 0;
}

/*************************
 * DMA memory allocation *
 *************************/

err_t mm_dma_allocate(size_t len, void **buffer)
{
struct dma_memory_entry *ptr = dma_memory_pool;

	while (ptr) {
		// Check length
		if (ptr->length < len)
			goto next;

		*buffer = (void *)ptr->start;

		// Are we allocating the entire entry?
		if (len == ptr->length) {
			// Free this entry
			mm_dma_entry_remove(ptr);
		} else {
			// Resize this entry
			ptr->start += len;
		}

		return 0;

	next:
		ptr = ptr->next;
	}

	*buffer = 0;
	return ERROR_NO_MEMORY;
}

/*
 * Initialization
 */
err_t mm_dma_init(void)
{
unsigned int i, mmap_entries;
err_t ret;
unsigned int start, len, entry_size;

	mmap_entries = _multiboot->mmap_length / sizeof(struct e820_map_entry);

	// Scan through every e820 map entry and take memory below 1MB
	for (i = 0; i < mmap_entries; i++) {
		#define entry 		_multiboot->mmap_entry[i]
		#define ROUND_LEN(len)	((len) & (DMA_PAGE_SIZE - 1))

		if (entry.type != typeAvailable || entry.base_addr > 0x100000)
			continue;

		// Add an entry for each area not spanning a page boundary
		start = entry.base_addr;
		len = entry.length;
		while (len) {
			entry_size = len > DMA_PAGE_SIZE ? DMA_PAGE_SIZE : len;
			mm_dma_entry_add(start, entry_size);
			start += entry_size;
			len -= entry_size;
		}
		
	}
		

	return 0;
}
