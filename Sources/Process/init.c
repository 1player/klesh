/*
 * Process/init.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-12-08
 *
 */

#include <process.h>
#include <types.h>
#include <cpu.h>
#include <mm.h>
#include <memory.h>
#include <interrupt.h>
#include <kernel.h>

/* From x86.asm */
extern union dt_entry _gdt[];
extern struct tss _kernel_tss;
extern void process_ltr(uint16_t descriptor);
extern uint32_t _process_page_directory[1024];
extern void _freeze(void);

err_t process_init(void)
{

struct thread *init_thread;
err_t ret;
uint32_t idle_stack;
struct process *kernel_process;

	total_processes = 0;
	total_threads = 0;

	/******************************
	 * Create the free PID bitmap *
	 ******************************/

	free_pid_bitmap_size = (PROCESS_MAX_PID + 1) / 8;
	free_pid_bitmap = (unsigned int *)mm_heap_allocate(free_pid_bitmap_size);
	memory_fill(free_pid_bitmap, 0xFFFFFFFF, free_pid_bitmap_size / 4);
	free_pid_bitmap[0] = ~1;			// PID 0 is used by the kernel process

	/*****************************
	 * Create the kernel process *
	 *****************************/

	current_process = kernel_process = (struct process *)mm_heap_allocate(sizeof(struct process));
	if (!kernel_process)
		kernel_panic("No memory to create the kernel process!");
	memory_clear(kernel_process, sizeof(struct process));

	kernel_process->name = "Kernel";
	kernel_process->pid = 0;
	kernel_process->page_directory = (uint32_t)&_process_page_directory;

	total_processes = 1;
	process_list = kernel_process;
	
	/*****************************
	 * Create the kernel threads *
	 *****************************/
	
	// Init thread (this one)
	current_thread = init_thread = (struct thread *)mm_heap_allocate(sizeof(struct thread));
	if (!init_thread)
		kernel_panic("Unable to initialize kernel threads! Error code %u", ERROR_NO_MEMORY);
	memory_clear(init_thread, sizeof(struct thread));
	init_thread->priority = priorityNormal;
	init_thread->parent = kernel_process;
	init_thread->status = statusReady;
	kernel_process->thread_list = init_thread;
	total_threads++;

	// Idle thread. It prevents the system from locking up when no more threads are available
	ret = process_thread_create(kernel_process, priorityIdle, (uint32_t)_freeze, PROCESS_THREAD_STACK_MIN);
	if (ret)
		kernel_panic("Unable to initialize kernel threads! Error code %u", ret);
	#if 0
	// Zombie slayer thread. Kills zombie threads
	ret = process_thread_create(kernel_process, priorityNormal, (uint32_t)process_thread_slayer, PROCESS_THREAD_STACK_MIN);
	if (ret)
		kernel_panic("Unable to initialize kernel threads! Error code %u", ret);
	#endif

	/*************************
	 * Create the kernel TSS *
	 *************************/

	/* Set up the descriptor */
	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.type = CPU_GDT_TYPE_TASK;
	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.access = CPU_GDT_ACCESS_PRESENT | CPU_GDT_ACCESS_DPL_0;
	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.flags = CPU_GDT_FLAG_AVAILABLE;

	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.limit_low = sizeof(struct tss) - 1;
	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.base_low = (unsigned int)&_kernel_tss & 0xFFFF;
	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.base_med = ((unsigned int)&_kernel_tss >> 16) & 0xFF;
	_gdt[CPU_GDT_INDEX_TSS_KERNEL].desc.base_high = ((unsigned int)&_kernel_tss >> 24) & 0xFF;

	/* Set up the TSS. */
	memory_clear(&_kernel_tss, sizeof(struct tss));
	
	_kernel_tss.ss0 = 0x10;
	_kernel_tss.esp0 = (uint32_t)mm_heap_allocate(4096);
	if (!_kernel_tss.esp0)
		kernel_panic("Not enough memory to create the kernel mode stack!");

	/*
	 * Now we can initialize the double fault handler
	 */
	ret = interrupt_init_doublefault();
	if (ret)
		kernel_panic("Unable to initialize the double fault handler! Error code %u", ret);
		
	console_write_formatted("%x\n", kernel_process->thread_list->priority);

	/*
	 * Load the Task Register with the kernel TSS descriptor
	 */

	process_ltr(CPU_GDT_INDEX_TSS_KERNEL * sizeof(union dt_entry));

	return 0;
}
