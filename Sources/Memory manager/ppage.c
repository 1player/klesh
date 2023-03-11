/*
 * Memory manager/ppage.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-11-26
 */

#include <mm.h>
#include <types.h>
#include <multiboot.h>
#include <console.h>
#include <bios.h>
#include <string.h>
#include <memory.h>

/* From kernel.ld */
extern void _end;

/* From x86.asm */
extern void _dummy_page_directory;

static uint32_t *ppage_stack_ptr = (uint32_t *)&_end;
static unsigned int total_pages = 0, free_pages = 0;


inline unsigned int mm_ppage_get_free(void)
{
	return free_pages;
}

inline unsigned int mm_ppage_get_total(void)
{
	return total_pages;
}

err_t mm_ppage_pop(uint32_t *ptr, size_t count)
{
/* Check MUST be done outside this function to improve performance.
	if (count > free_pages)
		return ERROR_NO_MEMORY;
*/

int i;

	ppage_stack_ptr -= count;
	free_pages -= count;

	memory_copy(ptr, ppage_stack_ptr, count * sizeof(uint32_t));

	return 0;
}

void mm_ppage_push(uint32_t *ptr, size_t count)
{
	memory_copy(ppage_stack_ptr, ptr, count * sizeof(uint32_t));

	ppage_stack_ptr += count;
	free_pages += count;

	/* We may be pushing memory that was not counted in mm_ppage_init (such as ACPI tables) so adjust total memory */
	if (free_pages > total_pages)
		total_pages = free_pages;
}


/******************
 * Initialization *
 ******************/

err_t mm_ppage_init_map(uint32_t *kernel_end)
{
unsigned int i, mmap_entries;

	mmap_entries = _multiboot->mmap_length / sizeof(struct e820_map_entry);

	/* First of all, sort the entries by their base address */
	int mmap_entry_sort(const void *first, const void *second)
	{
		if ( ((struct e820_map_entry *)first)->base_addr < ((struct e820_map_entry *)second)->base_addr )
			return -1;
		else if ( ((struct e820_map_entry *)first)->base_addr > ((struct e820_map_entry *)second)->base_addr )
			return 1;
		
		return 0;
	}

	memory_sort(_multiboot->mmap_entry, mmap_entries, sizeof(struct e820_map_entry), mmap_entry_sort);

	/* Now coalesce adjacent entries (issue found on some BIOSes) and sanity check */
	for (i = 0; i < mmap_entries - 1; i++) {
		#define first  _multiboot->mmap_entry[i]
		#define second _multiboot->mmap_entry[i + 1]

		if ((second.base_addr == 
			(first.base_addr + first.length)) && (second.type == first.type)) {
			first.length += second.length;
			second.type = typeRemoved;
		}
	}

	/* Now push available RAM on the stack (that is, memory > (0x100000 + kernel size)) */
	for (i = 0; i < mmap_entries; i++) {
		#define entry _multiboot->mmap_entry[i]

		if ((entry.type != typeAvailable) || (entry.base_addr < 0x100000))
			continue;

		if ( entry.base_addr < (unsigned int)&_end ) {
	
			/* If the area spans the kernel, remove the used area */
			if ( (entry.base_addr + entry.length) > (unsigned int)&_end ) {
				entry.length -= (unsigned int)&_end - entry.base_addr;
				entry.base_addr = (unsigned int)&_end;
			} else {
				continue;
			}

		}

		{
			unsigned int cur_start = (entry.base_addr + 0xFFF) & 0xFFFFF000;
			unsigned int cur_length = entry.length >> 12;

			while (cur_length--) {
				*ppage_stack_ptr++ = cur_start;
				total_pages++;
				cur_start += 0x1000;
			}
		}
	}

	/* Push the dummy page directory placeholder. More info about its use at x86.asm */
	*ppage_stack_ptr++ = (uint32_t)&_dummy_page_directory;
	total_pages++;

	free_pages = total_pages;

	*kernel_end = (uint32_t)ppage_stack_ptr;
		
	return 0;
}

err_t mm_ppage_init_size(uint32_t *kernel_end)
{
	console_write("Physical page stack initializiation with the BIOS memory size is not implemented.\n");

	return ERROR_NOT_IMPLEMENTED;
}

err_t mm_ppage_init(uint32_t *kernel_end)
{
	/* 
	 * The physical page stack is put at the end of the kernel and we put
	 * in the kernel end pointer the new pointer to the end of the kernel,
	 * that is, at the end of the (kernel + ppage stack) area.
	 * To know the size of the page stack, we must calculate the available pages
	 * given by the BIOS memory map or, as a last resort, by the BIOS-given memory
	 * size.
	 */
	
	/* Check if the loader gave us the BIOS memory map */
	if (_multiboot->flags & (1 << 6))
		return mm_ppage_init_map(kernel_end);
	else if (_multiboot->flags & 1)
		return mm_ppage_init_size(kernel_end);
	else {
		console_write("No BIOS memory map and no memory size. No alternative implementation has been written.\n");
		return ERROR_NOT_IMPLEMENTED;
	}
}
