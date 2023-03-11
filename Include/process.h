/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <cpu.h>
#include <types.h>

/* Sleep objects */

typedef struct {
	unsigned int 	waiter_count;
	struct thread 	**waiter;
} sleep_object_t;

#define INITIALIZED_SLEEP_OBJECT	{0, 0}


/* Thread and processes */

#define PROCESS_MAX_PID			65535
#define PROCESS_THREAD_STACK_MIN	CPU_PAGE_SIZE	// Minimum stack size
#define PROCESS_THREAD_STACK_DEFAULT	CPU_PAGE_SIZE
#define PROCESS_THREAD_STACK_MAX	0xFFFFFFFF	// Maximum stack size. XXX - Set this

enum processPriority { priorityIdle, priorityLow, priorityNormal, priorityHigh };
enum processStatus   { statusReady, statusSleeping, statusZombie };

struct thread {
	uint32_t		esp;
	uint32_t		kernel_esp;

	enum processPriority	priority;
	enum processStatus	status;
	sleep_object_t		*cur_sleep_object;

	struct process		*parent;

	struct thread		*previous;
	struct thread		*next;
};

struct process {
	unsigned char		*name;		/* Process name */
	unsigned int		pid;		/* Process ID */
	unsigned int		thread_count;	/* Threads count */

	uint32_t		page_directory;	/* Page directory physical address */

	struct thread		*thread_list;

	struct process		*previous;
	struct process		*next;
};

struct process	*process_list;

struct process 	*current_process;
struct thread	*current_thread;

unsigned int total_threads, total_processes;

unsigned int *free_pid_bitmap;
size_t free_pid_bitmap_size;

err_t process_init(void);

err_t process_create(unsigned char *name, unsigned char *path);

err_t process_thread_create(struct process *parent, enum processPriority priority, uint32_t eip, uint32_t stack_size);
err_t process_thread_terminate(struct thread *thread);

err_t process_thread_sleep_object(sleep_object_t *obj);
err_t process_thread_sleep_time(unsigned int ms);
err_t process_thread_wakeup_object(sleep_object_t *obj);

void process_schedule_disable(void);
void process_schedule_enable(void);

// Threads
void process_loader(void);
void process_thread_slayer(void);

#endif /* !defined KERNEL_PROCESS_H */
