/*
 * interrupt.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-12-06
 *
 */

#include <types.h>
#include <interrupt.h>
#include <cpu.h>
#include <memory.h>
#include <io.h>
#include <string.h>
#include <console.h>
#include <mm.h>

/* From x86.asm */
extern void _int0_handler(void);
extern void _int1_handler(void);
extern void _int2_handler(void);
extern void _int3_handler(void);
extern void _int4_handler(void);
extern void _int5_handler(void);
extern void _int6_handler(void);
extern void _int7_handler(void);
extern void _int10_handler(void);
extern void _int11_handler(void);
extern void _int12_handler(void);
extern void _int13_handler(void);
extern void _int14_handler(void);
extern void _int16_handler(void);
extern void _int17_handler(void);
extern void _int18_handler(void);
extern void _int_unhandled(void);
extern void _irq0_handler(void);
extern void _irq1_handler(void);
extern void _irq2_handler(void);
extern void _irq3_handler(void);
extern void _irq4_handler(void);
extern void _irq5_handler(void);
extern void _irq6_handler(void);
extern void _irq7_handler(void);
extern void _irq8_handler(void);
extern void _irq9_handler(void);
extern void _irq10_handler(void);
extern void _irq11_handler(void);
extern void _irq12_handler(void);
extern void _irq13_handler(void);
extern void _irq14_handler(void);
extern void _irq15_handler(void);
extern void _syscall_misc_trap(void);
extern void interrupt_lidt(struct idt_info *_idt_info);
extern union dt_entry _gdt[];
extern struct tss _double_fault_tss;
extern void _process_page_directory;
extern void _dummy_page_directory;

/* From start.asm */
extern void _freeze(void);

static union dt_entry _idt_table[256];
static struct idt_info _idt_info;
static uint16_t irq_mask = 0xFFFE;	/* Set bits are disabled IRQs. Enable only IRQ0 (timer) */
static struct {
	unsigned int count;
	void (**isr)(void);
} irq_handler_list[16];




/* Generic exception function.
 *
 * number - Interrupt number address.
 * error_code - Optional processor error code. Exception-dependent
 * address - Used only when a page fault is issued and contains the faulting address (contents of CR2)
 * selector - code selector of the faulting code
 * eip - code offset of the faulting code
 */
void interrupt_trap_exception(unsigned number, uint32_t error_code, uint32_t address, uint16_t selector, uint32_t eip)
{
	if (number == -1)
		console_write_formatted("\nUnhandled exception @ %#.2X:%.8X: error code %.8X, CR2 %#.8X\n", 
			number, selector, eip, error_code, address);
	else
		console_write_formatted("\nException #%u @ %#.2X:%.8X: error code %.8X, CR2 %#.8X\n", 
			number, selector, eip, error_code, address);

	_freeze();
}

/*
 * Trap an Interrupt ReQuest
 */
void interrupt_trap_irq(unsigned number)
{
int i;
char buf[64];

	/* Call all the handlers for this IRQ */
	
	if (!irq_handler_list[number].count)
		return;
		
	for (i = 0; i < irq_handler_list[number].count; i++)
		irq_handler_list[number].isr[i]();
}

/*
 * Double fault handler.
 * The processor calls it on with a task switch.
 */
static void interrupt_trap_doublefault(void)
{
	interrupt_disable();

	console_write("\nDouble fault! This is really bad!\n");

	_freeze();
	
}




void interrupt_set_handler(uint8_t number, void (*function)(void), unsigned access)
{
	_idt_table[number].gate.offset_low = ((uint32_t)function) & 0xFFFF;
	_idt_table[number].gate.selector = 0x0008;
	_idt_table[number].gate.type = CPU_IDT_TYPE_INTERRUPT;
	_idt_table[number].gate.reserved = 0;
	_idt_table[number].gate.access = access;
	_idt_table[number].gate.offset_high = (((uint32_t)function) >> 16) & 0xFFFF;
}

static void interrupt_set_trap(uint8_t number, void (*function)(void), unsigned access)
{
	_idt_table[number].gate.offset_low = ((uint32_t)function) & 0xFFFF;
	_idt_table[number].gate.selector = 0x0008;
	_idt_table[number].gate.type = CPU_IDT_TYPE_TRAP;
	_idt_table[number].gate.reserved = 0;
	_idt_table[number].gate.access = access;
	_idt_table[number].gate.offset_high = (((uint32_t)function) >> 16) & 0xFFFF;
}

static void interrupt_set_task(uint8_t number, uint16_t task, unsigned flags)
{
	_idt_table[number].gate.offset_low = 0;
	_idt_table[number].gate.selector = task;
	_idt_table[number].gate.type = CPU_IDT_TYPE_TASK;
	_idt_table[number].gate.reserved = 0;
	_idt_table[number].gate.access = flags;
	_idt_table[number].gate.offset_high = 0;	
}



/********
 * IRQs *
 ********/

err_t interrupt_irq_register(uint8_t number, void (*isr)(void))
{
unsigned int count;

#define count	irq_handler_list[number].count

	if (number > 15)
		return ERROR_OUT_OF_BOUNDS;

	
	/* Add the IRQ handler to the handler list */
	irq_handler_list[number].isr = mm_heap_reallocate(irq_handler_list[number].isr,
						(count + 1) * sizeof(irq_handler_list[number].isr));
	if (!irq_handler_list[number].isr)
		return ERROR_NO_MEMORY;

	irq_handler_list[number].isr[count] = isr;
	count++;

	return 0;
}

err_t interrupt_irq_enable(uint8_t number)
{
	if (number > 15)
		return ERROR_OUT_OF_BOUNDS;

	irq_mask &= ~(1 << number);

	if (number > 8)
		irq_mask &= ~(1 << 2);

	port_write_byte(INTERRUPT_8259_MASTER_DATA, irq_mask & 0xFF);
	port_write_byte(INTERRUPT_8259_SLAVE_DATA, (irq_mask >> 8) & 0xFF);
	
	return 0;
}

err_t interrupt_irq_disable(uint8_t number)
{
	if (number > 15)
		return ERROR_OUT_OF_BOUNDS;
		
	irq_mask |= 1 << number;

	port_write_byte(INTERRUPT_8259_MASTER_DATA, irq_mask & 0xFF);
	port_write_byte(INTERRUPT_8259_SLAVE_DATA, (irq_mask >> 8) & 0xFF);
	
	return 0;
}


/******************
 * Initialization *
 ******************/

/* Initialize the double fault handler */
err_t interrupt_init_doublefault(void)
{
	if (mm_ppage_get_free() < 1)
		return ERROR_NO_MEMORY;

	/* Set up the descriptor */
	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.type = CPU_GDT_TYPE_TASK;
	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.access = CPU_GDT_ACCESS_PRESENT | CPU_GDT_ACCESS_DPL_0;
	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.flags = CPU_GDT_FLAG_AVAILABLE;

	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.limit_low = sizeof(struct tss) - 1;
	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.base_low = (unsigned int)&_double_fault_tss & 0xFFFF;
	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.base_med = ((unsigned int)&_double_fault_tss >> 16) & 0xFF;
	_gdt[CPU_GDT_INDEX_TSS_DOUBLE_FAULT].desc.base_high = ((unsigned int)&_double_fault_tss >> 24) & 0xFF;

	/* Set up the TSS */
	memory_clear(&_double_fault_tss, sizeof(struct tss));

	_double_fault_tss.eip = (uint32_t)&interrupt_trap_doublefault;
	_double_fault_tss.cs = CPU_GDT_INDEX_KERNEL_CS * sizeof(union dt_entry);
	_double_fault_tss.ds = _double_fault_tss.es =_double_fault_tss.fs = _double_fault_tss.ss = 
		_double_fault_tss.ss0 = CPU_GDT_INDEX_KERNEL_DS * sizeof(union dt_entry);
	_double_fault_tss.esp = _double_fault_tss.esp0 = (uint32_t)mm_heap_allocate_aligned(4, 4096) + 4096;
	_double_fault_tss.eflags = 0x200;
	
	// Setup its page directory
	mm_ppage_pop(&_double_fault_tss.cr3, 1);
	mm_map_page_directory(_double_fault_tss.cr3);
	memory_clear(&_dummy_page_directory, CPU_PAGE_SIZE);
	memory_copy(&_dummy_page_directory, &_process_page_directory, (MM_AREA_KERNEL_END / 0x400000) * sizeof(uint32_t));

	/* Set up the interrupt handler */
	interrupt_set_task(8, CPU_GDT_INDEX_TSS_DOUBLE_FAULT * sizeof(union dt_entry), CPU_IDT_ACCESS_PRESENT | CPU_IDT_ACCESS_DPL_0);

	return 0;
}

void interrupt_init_syscalls(void)
{
#define TRAP_FLAGS 	(CPU_IDT_ACCESS_PRESENT | CPU_IDT_ACCESS_DPL_3)

	interrupt_set_trap(0x80, _syscall_misc_trap, TRAP_FLAGS);
}

err_t interrupt_init(void)
{
unsigned i;

	/*
	 * Clear the IDT table and the IRQ handler list
	 */

	memory_clear(_idt_table, sizeof(_idt_table));
	memory_clear(irq_handler_list, sizeof(irq_handler_list));

	/*
	 * Start by initializing the processor exceptions
	 * They are defined in x86.asm
	 */

#define FLAGS 		(CPU_IDT_ACCESS_PRESENT | CPU_IDT_ACCESS_DPL_0)
	
	interrupt_set_handler(0, _int0_handler, FLAGS);		/* Divide error */
	interrupt_set_handler(1, _int1_handler, FLAGS);		/* Debug exception */
	interrupt_set_handler(2, _int2_handler, FLAGS);		/* NMI */
	interrupt_set_handler(3, _int3_handler, FLAGS);		/* Breakpoint */
	interrupt_set_handler(4, _int4_handler, FLAGS);		/* Overflow */
	interrupt_set_handler(5, _int5_handler, FLAGS);		/* Bounds check */
	interrupt_set_handler(6, _int6_handler, FLAGS);		/* Invalid opcode */
	interrupt_set_handler(7, _int7_handler, FLAGS);		/* Coprocessor not available */
	//interrupt_set_task(8, CPU_GDT_INDEX_TSS_DOUBLE_FAULT * sizeof(union dt_entry), CPU_IDT_ACCESS_PRESENT | CPU_IDT_ACCESS_DPL_0);
	interrupt_set_handler(9, _int_unhandled, FLAGS);	/* Coprocessor segment overrun */
	interrupt_set_handler(10, _int10_handler, FLAGS);	/* Invalid TSS */
	interrupt_set_handler(11, _int11_handler, FLAGS);	/* Segment not present */
	interrupt_set_handler(12, _int12_handler, FLAGS);	/* Stack exception */
	interrupt_set_handler(13, _int13_handler, FLAGS);	/* General protection fault */
	interrupt_set_handler(14, _int14_handler, FLAGS);	/* Page fault */
	interrupt_set_handler(15, _int_unhandled, FLAGS);
	interrupt_set_handler(16, _int16_handler, FLAGS);	/* Coprocessor error */
	interrupt_set_handler(17, _int17_handler, FLAGS);	/* Alignment check */
	interrupt_set_handler(18, _int18_handler, FLAGS);	/* Machine check */

	for (i = 19; i < 32; i++)
		interrupt_set_handler(i, _int_unhandled, FLAGS);

	/* IRQs. */ 
	interrupt_set_handler(0x20, _irq0_handler, FLAGS);
	interrupt_set_handler(0x21, _irq1_handler, FLAGS);
	interrupt_set_handler(0x22, _irq2_handler, FLAGS);
	interrupt_set_handler(0x23, _irq3_handler, FLAGS);
	interrupt_set_handler(0x24, _irq4_handler, FLAGS);
	interrupt_set_handler(0x25, _irq5_handler, FLAGS);
	interrupt_set_handler(0x26, _irq6_handler, FLAGS);
	interrupt_set_handler(0x27, _irq7_handler, FLAGS);
	interrupt_set_handler(0x28, _irq8_handler, FLAGS);
	interrupt_set_handler(0x29, _irq9_handler, FLAGS);
	interrupt_set_handler(0x2A, _irq10_handler, FLAGS);
	interrupt_set_handler(0x2B, _irq11_handler, FLAGS);
	interrupt_set_handler(0x2C, _irq12_handler, FLAGS);
	interrupt_set_handler(0x2D, _irq13_handler, FLAGS);
	interrupt_set_handler(0x2E, _irq14_handler, FLAGS);
	interrupt_set_handler(0x2F, _irq15_handler, FLAGS);

	for (i = 0x30; i < 256; i++)
		interrupt_set_handler(i, _int_unhandled, FLAGS);
		
	interrupt_init_syscalls();


	/*
	 * Now we have to initialize the 8259
	 * Programmable Interrupt Controller 
	 */
	
	
	port_write_byte(INTERRUPT_8259_MASTER_CMD, 0x11);	/* Select master PIC init */
	port_write_byte(INTERRUPT_8259_MASTER_DATA, 0x20);	/* Master mapped to interrupts 0x20-0x27 */
	port_write_byte(INTERRUPT_8259_MASTER_DATA, 0x04);	/* Slave on IRQ2 */
	port_write_byte(INTERRUPT_8259_MASTER_DATA, 0x03);	/* Auto EOI */
	
	port_write_byte(INTERRUPT_8259_SLAVE_CMD, 0x11);	/* Select slave PIC init */
	port_write_byte(INTERRUPT_8259_SLAVE_DATA, 0x28);	/* Slave mapped to interrupts 0x28-0x2f */
	port_write_byte(INTERRUPT_8259_SLAVE_DATA, 0x02);	/* Slave on master's IRQ2 */
	port_write_byte(INTERRUPT_8259_SLAVE_DATA, 0x01);	/* Auto EOI */
	
	port_write_byte(INTERRUPT_8259_MASTER_DATA, irq_mask & 0xFF);		/* Disable all master IRQs */
	port_write_byte(INTERRUPT_8259_SLAVE_DATA, (irq_mask >> 8) & 0xFF);	/* Disable all slave IRQs */


	/*
	 * Now that everything is set up
	 * we can load our IDT and enable interrupts
	 */	
	 
	_idt_info.length = 256 * 8 - 1;
	_idt_info.addr = (uint32_t)_idt_table;

	interrupt_lidt(&_idt_info);

	return 0;
}
