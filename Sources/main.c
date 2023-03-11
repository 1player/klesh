/*
 * main.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-11-23
 *
 */

#include <console.h>
#include <cpu.h>
#include <mm.h>
#include <interrupt.h>
#include <process.h>
#include <timer.h>
#include <kernel.h>
#include <modules.h>
#include <multiboot.h>
#include <elf.h>
#include <memory.h>

extern void _dummy_page_directory, _process_page_directory;
extern struct process *kernel_process;


static void shell_loader(void)
{
void *shell_handle;
Elf32_Ehdr header;
Elf32_Phdr program_header;
unsigned int pos, flags;

	if (ext2_open("/system/shell.x", &shell_handle)) {
		console_write("Cannot open the shell executable\n");
		return;
	}
	
	// Read the ELF header
	ext2_read(shell_handle, (unsigned char *)&header, 0, sizeof(Elf32_Ehdr));
	if (elf_check_header(&header) || header.e_type != ET_EXEC) {
		console_write("Invalid shell executable format!\n");
		return;
	}

	// Copy to memory the program sections
	pos = header.e_phoff;
	while (header.e_phnum--) {
		ext2_read(shell_handle, (unsigned char *)&program_header, pos, sizeof(Elf32_Phdr));
		
		// Process only loadable sections
		if (!(program_header.p_type == PT_LOAD))
			goto next;
		
		flags = (program_header.p_flags & PF_W ? CPU_PAGE_FLAG_WRITABLE : 0);
		flags |= CPU_PAGE_FLAG_USER;
		
		// Allocate page if not present
		if (mm_map_check(program_header.p_vaddr >> CPU_PAGE_SHIFT)) 
			mm_map(program_header.p_vaddr >> CPU_PAGE_SHIFT, size_in_pages(program_header.p_memsz), flags);
		
		// Copy the file data to memory, if any
		if (program_header.p_filesz) 
			ext2_read(shell_handle, (void *)program_header.p_vaddr, program_header.p_offset, program_header.p_filesz);

		// Clear the padding (the difference between size in memory and size in file)			
		memory_clear((void *)(program_header.p_vaddr + program_header.p_filesz), program_header.p_memsz - program_header.p_filesz);
		
	next:
		pos += sizeof(Elf32_Phdr);
		
	}
	
	// Allocate the user stack
	mm_map(0xFFFFF, 1, CPU_PAGE_FLAG_USER | CPU_PAGE_FLAG_WRITABLE);
		
	ext2_close(shell_handle);
	
	cpu_usermode(header.e_entry);
}



struct boot_module {
	err_t (*func)(void);
	unsigned char *title;
};

static struct boot_module first_stage_module[] = {
	{ interrupt_init, "Interrupts" },
	{ cpu_init, "CPU detection" },
	{ mm_init, "Memory manager" },
	{ process_init, "Multitasking subsystem" },
	{ timer_init, "System timer" }
};
#define first_stage_module_count		(sizeof(first_stage_module) / sizeof(struct boot_module))

static struct boot_module system_module[] = {
	{ keyboard_init, "Keyboard driver" },
	{ fdc_init, "Floppy disk driver" },
	{ ata_init, "ATA/ATAPI disk driver" },
	{ ext2_init, "Ext2 file system" }
};
#define system_module_count		(sizeof(system_module) / sizeof(struct boot_module))

void main(unsigned int multiboot_magic)
{
unsigned int i;
struct process *shell;

	console_init();

	console_clear();
	console_boot_init(first_stage_module_count + system_module_count);

	/******************************
	 * First stage initialization *
	 ******************************/

	for (i = 0; i < first_stage_module_count; i++) {
		console_boot_step(first_stage_module[i].title);
		first_stage_module[i].func();
	}

	interrupt_enable();

	/*************************
	 * System initialization *
	 *************************/

	for (i = 0; i < system_module_count; i++) {
		console_boot_step(system_module[i].title);
		system_module[i].func();
	}

	console_clear();
	console_cursor_show(1);

	if ((_multiboot->flags & 1) == 0) {
		// XXX - Kernel panic, etc. etc.
	}

	if (_multiboot->boot_drive & 0x80) {
		// Load from the hard disk
		console_write("Loading the system from a hard disk is not supported yet.\n");
		return;
	} else {
		// Load from the floppy disk
		if (_multiboot->boot_drive & 0x7F) {
			console_write("System loading is supported only from the first floppy disk\n");
			return;
		}
	}

	/*****************
	 * Shell loading *
	 *****************/
	 
	console_write("Loading shell...\n");
	
	
	// Create shell process. We still cannot use process_create.
	if (mm_ppage_get_free() < 1)
		goto fail;

	shell = (struct process *)mm_heap_allocate(sizeof(struct process));
	if (!shell)
		goto fail;
	memory_clear(shell, sizeof(struct process));

	// Initialize process structure
	shell->name = "Shell";
	shell->pid = 2;
	
	// Create process loading thread
	if (process_thread_create(shell, priorityNormal, (uint32_t)shell_loader, PROCESS_THREAD_STACK_DEFAULT)) {
		mm_heap_free(shell);
		goto fail;
	}
	
	interrupt_disable();
	
	// Initialize process memory
	mm_ppage_pop(&shell->page_directory, 1);
	mm_map_page_directory(shell->page_directory);
	memory_clear(&_dummy_page_directory, CPU_PAGE_SIZE);
	memory_copy(&_dummy_page_directory, &_process_page_directory, (MM_AREA_KERNEL_END / 0x400000) * sizeof(uint32_t));
	
	// Map the page directory itself at 0x02000000
	{
	uint32_t *page_dir;
	
		page_dir = (uint32_t *)&_dummy_page_directory;
		page_dir[0x02000000 >> 22] = shell->page_directory | 0x3;
	}
	
	// Map the process page directory on the page directory access window
	{
	uint32_t page_table;
	uint32_t *table;
	
		page_table = (uint32_t)( ((uint32_t *)&_dummy_page_directory)[(uint32_t)&_process_page_directory >> 22] & 0xFFFFF000 );
		mm_map_page_directory(page_table);
		table = (uint32_t *)&_dummy_page_directory;
		table[((uint32_t)&_process_page_directory >> 12) & 0x3FF] = shell->page_directory | 0x3;
	}
	
	// Put the process in the process list
	process_list->previous = shell;
	shell->next = process_list;
	process_list = shell;
	
	total_processes++;
	
	interrupt_enable();
	
	process_thread_terminate(current_thread);
	
fail:
	console_write("Shell loading failed. Aborting.\n");
	process_thread_terminate(current_thread);
}
