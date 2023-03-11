/*
 * Process/thread.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-12-20
 *
 */

#include <process.h>
#include <memory.h>
#include <mm.h>
#include <console.h>
#include <cpu.h>
#include <interrupt.h>
#include <timer.h>

/* Sleep and wakeups */

extern void process_thread_reschedule(uint32_t eflags);

err_t process_thread_sleep_object(sleep_object_t *obj)
{
uint32_t eflags;

	eflags = cpu_flags_get();
	interrupt_disable();

	// Resize the waiter list
	obj->waiter = (struct thread **)mm_heap_reallocate(obj->waiter, sizeof(struct thread *) * (obj->waiter_count + 1));
	if (!obj->waiter)
		return ERROR_NO_MEMORY;

	// The current thread _cannot_ be sleeping already */
	if (current_thread->cur_sleep_object || current_thread->status != statusReady) {
		console_write("BUG: The thread is already sleeping on an object!\n");
		while (1);
	}

	// Put the current thread on the waiter list and set it sleeping
	obj->waiter[obj->waiter_count] = current_thread;
	current_thread->cur_sleep_object = obj;
	current_thread->status = statusSleeping;

	obj->waiter_count++;

	// Reschedule
	process_thread_reschedule(eflags);
	
	return 0;
}

err_t process_thread_sleep_time(unsigned int ms)
{
unsigned int end_time;
uint32_t eflags;

	eflags = cpu_flags_get();
	interrupt_disable();
	
	if (ms) {

		// Convert the timeout from ms to timer ticks
		ms /= TIMER_GRANULARITY_MS;
		end_time = _ticks + ms;
		
		while (_ticks < end_time)
			process_thread_reschedule(eflags);
	} else {
		
		process_thread_reschedule(eflags);
		
	}

	return 0;
}

err_t process_thread_wakeup_object(sleep_object_t *obj)
{
unsigned int i;

	interrupt_disable();

	// Wake up all the waiter threads
	for (i = 0; i < obj->waiter_count; i++) {
		obj->waiter[i]->status = statusReady;
		obj->waiter[i]->cur_sleep_object = 0;
	}

	// Free the waiter list and reset the count
	mm_heap_free(obj->waiter);
	obj->waiter_count = 0;

	interrupt_enable();

	return 0;
}

/* Zombie slayer thread. Don't call directly */

void process_thread_slayer(void)
{
struct thread *thread;
struct process *process;

	process = process_list;
	thread = process->thread_list;
	while (1) {
		if (thread->status == statusZombie) {
			interrupt_disable();
			if (thread->next)
				thread->next->previous = thread->previous;
			if (thread->previous)
				thread->previous->next = thread->next;
			interrupt_enable();
			
			mm_heap_free(thread);
			total_threads--;
			process->thread_count--;
		}
		
		if (thread->next)
			thread = thread->next;
		else if (process->next) {
			process = process->next;
			thread = process->thread_list;
		} else {
			process_thread_sleep_time(0);
		}			
	}
}

/* Thread creation */

err_t process_thread_create(struct process *parent, enum processPriority priority, uint32_t eip, uint32_t stack_size)
{
struct thread *thread;
err_t ret;

	// Sanity check on the input
	if (!parent || (stack_size < PROCESS_THREAD_STACK_MIN) || (stack_size > PROCESS_THREAD_STACK_MAX))
		return ERROR_INVALID;
	
	thread = (struct thread *)mm_heap_allocate(sizeof(struct thread));
	if (!thread)
		return ERROR_NO_MEMORY;
	memory_clear(thread, sizeof(struct thread));

	thread->priority = priority;
	thread->parent = parent;
	thread->status = statusReady;

	/* Allocate a stack for the new thread */
	thread->esp = (uint32_t)mm_heap_allocate(stack_size);
	if (!thread->esp) {
		mm_heap_free(thread);
		return ERROR_NO_MEMORY;
	}
	
	memory_clear((void *)thread->esp, stack_size);
	thread->esp += stack_size - 11 * 4;
	*(uint32_t *)(thread->esp + 4*8) = eip;
	*(uint32_t *)(thread->esp + 4*9) = 0x08;
	*(uint32_t *)(thread->esp + 4*10) = 0x200;
	
	/* Allocate a kernel stack */
	thread->kernel_esp = (uint32_t)mm_heap_allocate(PROCESS_THREAD_STACK_DEFAULT);
	if (!thread->kernel_esp) {
		mm_heap_free((void *)thread->esp);
		mm_heap_free(thread);
		return ERROR_NO_MEMORY;
	}

	thread->next = parent->thread_list;
	thread->previous = 0;
	parent->thread_list->previous = thread;
	parent->thread_list = thread;
	parent->thread_count++;

	total_threads++;

	return 0;
}

/* Thread kill */

err_t process_thread_terminate(struct thread *thread)
{
uint32_t eflags;

	eflags = cpu_flags_get();
	interrupt_disable();
	
	thread->status = statusZombie;
	
	process_thread_reschedule(eflags);
	
	return 0;
}
