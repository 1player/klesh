/*
 * Memory manager/map.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-12-09
 *	2005-05-15: Map support on dummy page directory
 */

#include <mm.h>
#include <cpu.h>
#include <memory.h>
#include <kernel.h>

/* From x86.asm */
extern uint32_t _process_page_directory[1024];
extern uint32_t _dummy_page_directory[1024];

/* Get an existing page table or create a new one if not present */
static uint32_t *get_create_page_table(unsigned index, unsigned flags)
{
uint32_t *page_table, *page_directory;

	page_table = (uint32_t *)(0x02000000 + (index << 12));
	page_directory = _process_page_directory;
		
	if (!(page_directory[index] & CPU_PAGE_FLAG_PRESENT)) {
		mm_ppage_pop(&page_directory[index], 1);
		page_directory[index] |= flags;

		memory_clear(page_table, CPU_PAGE_SIZE);
	}

	return page_table;
}

/* Get an existing page table  */
static uint32_t *get_page_table(unsigned index)
{
uint32_t *page_table, *page_directory;

	page_directory = _process_page_directory;

	if (page_directory[index] & CPU_PAGE_FLAG_PRESENT)
		return (uint32_t *)(0x02000000 + (index << 12));
	
	return (uint32_t *)0;
}

/*
 * Pay attention not to use mm_map and call mm_unmap_linear to free them, or
 * mm_map_linear and mm_unmap!
 */

err_t mm_map(uint32_t start, uint32_t length, unsigned flags)
{
uint32_t *page_table;
uint32_t start_table, end_table;
uint32_t start_page, end_page;
uint32_t count, i;

	assert(!(flags & 0xFFFFF000));
	flags |= CPU_PAGE_FLAG_PRESENT;

	start_table = start >> 10;
	end_table = (start + length) >> 10;

	/* Check if there is enough memory left. Include in the computation the optionally needed page tables */
	if ( mm_ppage_get_free() < (length + (length >> 10 + 1)) )
		return ERROR_NO_MEMORY;

	for (i = start_table; i <= end_table; i++) {
		/* Get the page table and if it doesn't exist, create one */
		page_table = get_create_page_table(i, flags);

		start_page = max(start_table * 1024, start) & 0x3FF;
		count = min(1024 - start_page, length);

		mm_ppage_pop(&page_table[start_page], count);

		length -= count;

		/* Set flags */
		while (count--)
			page_table[start_page++] |= flags;
	}

	return 0;
}

err_t mm_map_physical(uint32_t virtual, uint32_t physical, uint32_t length, unsigned flags)
{
uint32_t *page_table;
uint32_t start_table, end_table;
uint32_t start_page, end_page;
uint32_t count, i;

	assert(!(flags & 0xFFFFF000));
	flags |= CPU_PAGE_FLAG_PRESENT;
	physical <<= 12;

	start_table = virtual >> 10;
	end_table = (virtual + length) >> 10;

	for (i = start_table; i <= end_table; i++) {
		/* Get the page table and if it doesn't exist, create one */
		page_table = get_create_page_table(i, flags);

		start_page = max(start_table * 1024, virtual) & 0x3FF;
		count = min(1024 - start_page, length);

		while (count--) {
			page_table[start_page++] = physical | flags;
			physical += 0x1000;
		}

		length -= count;
	}

	return 0;
}




void mm_unmap(uint32_t start, uint32_t length)
{
uint32_t *page_table;
uint32_t start_table, end_table;
uint32_t start_page, end_page;
uint32_t end, count, i;

	start_table = start >> 10;
	end_table = (start + length) >> 10;
	end = start + length;

	for (i = start_table; i < end_table; i++) {
		/* Get the page table */
		page_table = get_page_table(i);
		assert(page_table);

		start_page = max(start_table * 1024, start);
		count = min(1024, end);

		/* Remove flags */
		while (count--)
			page_table[start_page++] &= 0xFFFFF000;

		start_page = max(start_table * 1024, start);
		count = min(1024, end);

		mm_ppage_push(&page_table[start_page], count);

		end -= count;
	}

	cpu_mmu_invalidate(start, length);
}

void mm_unmap_physical(uint32_t start, uint32_t length)
{
uint32_t *page_table;
uint32_t start_table, end_table;
uint32_t start_page, end_page;
uint32_t end, count, i;

	start_table = start >> 10;
	end_table = (start + length) >> 10;
	end = start + length;

	for (i = start_table; i < end_table; i++) {
		/* Get the page table */
		page_table = get_page_table(i);
		assert(page_table);

		start_page = max(start_table * 1024, start);
		count = min(1024, end);

		memory_clear(&page_table[start_page], count * sizeof(uint32_t));

		end -= count;
	}

	cpu_mmu_invalidate(start, length);
}


err_t mm_map_page_directory(uint32_t page_dir)
{
err_t ret;

	// Map it on the dummy page directory placeholder
	ret = mm_map_physical((uint32_t)&_dummy_page_directory >> 12, page_dir >> 12, 1, CPU_PAGE_FLAG_WRITABLE);
	
	// An invalidate is needed because the map function does not invalidate the caches and
	// no unmap will ever be called for a page directory map.
	cpu_mmu_invalidate((uint32_t)&_dummy_page_directory >> 12, 1);
	
	if (ret)
		return ret;

	return 0;
}

err_t mm_map_check(uint32_t page)
{
uint32_t *page_table;

	page_table = get_page_table(page >> 10);
	if (!page_table)
		return ERROR_NOT_FOUND;
	
	if (page_table[page & 0x3FF] & CPU_PAGE_FLAG_PRESENT)
		return 0;
		
	return ERROR_NOT_FOUND;
}


