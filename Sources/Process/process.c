/*
 * Process/process.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-05-08
 *
 */

#include <process.h>
#include <memory.h>
#include <mm.h>
#include <bit.h>
#include <console.h>
#include <cpu.h>
#include <interrupt.h>

extern void _dummy_page_directory, _process_page_directory;

unsigned int alloc_pid(void)
{
unsigned int i, ret;

	for (i = 0; i < (free_pid_bitmap_size / 4); i++) {
		if (!free_pid_bitmap[i])
			continue;

		ret = bit_find_set(free_pid_bitmap[i]);
		free_pid_bitmap[i] = bit_reset(free_pid_bitmap[i], ret);
		return (ret + 32 * i);
	}

	return -1;
}

err_t process_create(unsigned char *name, unsigned char *path)
{
struct process *process;

	if (mm_ppage_get_free() < 1)
		return ERROR_NO_MEMORY;

	process = (struct process *)mm_heap_allocate(sizeof(struct process));
	if (!process)
		return ERROR_NO_MEMORY;
	memory_clear(process, sizeof(struct process));

	// Initialize process structure
	process->name = name;
	process->pid = alloc_pid();
	
	// Create process loading thread
	if (process_thread_create(process, priorityNormal, (uint32_t)process_loader, PROCESS_THREAD_STACK_DEFAULT)) {
		mm_heap_free(process);
		return ERROR_NO_MEMORY;
	}
	
	// Initialize process memory
	mm_ppage_pop(&process->page_directory, 1);
	mm_map_page_directory(process->page_directory);
	memory_clear(&_dummy_page_directory, CPU_PAGE_SIZE);
	memory_copy(&_dummy_page_directory, &_process_page_directory, (MM_AREA_KERNEL_END / 0x400000) * sizeof(uint32_t));
	
	// Put the process in the process list
	interrupt_disable();
	process_list->previous = process;
	process->next = process_list;
	process_list = process;
	interrupt_enable();
	
	total_processes++;

	return 0;
}

err_t process_terminate(struct process *process)
{
	return 0;
}
