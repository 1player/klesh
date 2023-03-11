/*
 * Memory manager/init.c
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
#include <memory.h>
#include <cpu.h>

/* From x86.asm */
extern uint32_t _process_page_directory[1024];
extern void _dummy_page_directory;
extern void cpu_paging_enable(unsigned int page_dir_addr);


static void allocate_kernel_page_tables(uint32_t start_table, uint32_t length, unsigned int flags)
{
uint32_t table;

	while (length--) {
		mm_ppage_pop(&table, 1);
		_process_page_directory[start_table++] = table | flags;
		memory_clear((void *)table, CPU_PAGE_SIZE);
	}
}

static void map(uint32_t pstart, uint32_t vstart, size_t pages, unsigned int flags)
{
uint32_t *table_ptr;
uint32_t vend;
unsigned int i, j, count;

	vend = vstart + pages;

	for (i = (vstart >> 10); i <= (vend >> 10); i++) {
		/* No need to allocate a page table since we allocated all the needed for the kernel area */
		table_ptr = (uint32_t *)(_process_page_directory[i] & 0xFFFFF000);

		count = min(pages, 1024);
		pages -= count;
		
		while (count--) {
			table_ptr[vstart++ & 0x3FF] = (pstart++ << 12) | flags;
		}
	}
}

err_t mm_init(void)
{
err_t ret;
uint32_t kernel_end;
unsigned int flags;

	/*
	 * Initialize memory manager subsystems
	 */

	ret = mm_ppage_init(&kernel_end);
	if (ret)
		return ret;

	/*
	 * Setup the kernel page directory
	 */

	memory_clear(_process_page_directory, CPU_PAGE_SIZE);

	/* Set kernel memory to writable and global (IOW, those pages won't be cleared from the cache) if the CPU supports it */
	flags = CPU_PAGE_FLAG_WRITABLE | CPU_PAGE_FLAG_PRESENT;
	if (_cpu.capabilities & CPU_CAPABILITY_GLOBALPAGES)
		flags |= CPU_PAGE_FLAG_GLOBAL;

	/* Allocate page tables for the entire kernel area, so every process has them all and any modification in the
	 * kernel memory is automagically reflected on every process of the system */
	allocate_kernel_page_tables(MM_AREA_KERNEL_START >> 22, (MM_AREA_KERNEL_END - MM_AREA_KERNEL_START) >> 22, flags);

	/* Map the entire kernel and the low memory into virtual memory */
	map(0x00000, 0x00000, size_in_pages(kernel_end), flags);

	/* Map the page directory itself at 0x02000000 */
	_process_page_directory[0x02000000 >> 22] = (uint32_t)_process_page_directory | flags;

	/*
	 * Enable paging
	 */

	cpu_paging_enable((unsigned int)_process_page_directory);
	
	mm_unmap_physical((uint32_t)&_dummy_page_directory >> 12, 1);

	ret = mm_dma_init();
	if (ret)
		return ret;

	return 0;
}
