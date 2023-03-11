/*
 * Process/schedule.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-12-20
 *
 */

#include <process.h>
#include <mm.h>
#include <cpu.h>

extern struct process *kernel_process;

void process_schedule_disable(void)
{
	interrupt_irq_disable(0);
}

void process_schedule_enable(void)
{
	interrupt_irq_enable(0);
}

/* process_schedule - Selects the next thread to run and switches.
*/
struct thread *process_schedule(void)
{
struct thread *old_thread;
struct process *old_process;

	old_thread = current_thread;
	old_process = current_process;
	
	current_thread = old_thread->next;
	while (1) {
		// Find the next ready thread in the current process.
		while (current_thread) {
			if (current_thread->status == statusReady)
				goto found;

			current_thread = current_thread->next;
		}

		// No threads found in the current process, so switch
		if (current_process->next)
			current_process = current_process->next;
		else
			current_process = process_list;		// Restart from the first process

		current_thread = current_process->thread_list;
	}

found:
	// If we have switched the process, change the page directory
	if (old_process != current_process)
		cpu_mmu_switch(current_process->page_directory);

	return old_thread;
}
