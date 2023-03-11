/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_CPU_H
#define KERNEL_CPU_H

#include <types.h>

/*******
 * MMU *
 *******/

/* Page flags */

#define CPU_PAGE_FLAG_PRESENT	0x001
#define CPU_PAGE_FLAG_WRITABLE	0x002
#define CPU_PAGE_FLAG_USER	0x004
#define CPU_PAGE_FLAG_GLOBAL	0x100

#define CPU_PAGE_SIZE		4096
#define CPU_PAGE_SHIFT		12

#define size_in_pages(x)	((((x) + 0xFFF) & 0xFFFFF000) >> 12)




/*************************
 * Processor information *
 *************************/

enum cpu_vendor { vendorIntel = 0, vendorUMC, vendorAMD, vendorCyrix, vendorNexGen, vendorCentaur,
			vendorRise, vendorSiS, vendorTransmeta, vendorNSC, vendorUnknown = -1 };

/* Processor capabilities */
#define CPU_CAPABILITY_GLOBALPAGES	0x00000001
#define CPU_CAPABILITY_TIMESTAMPCOUNTER	0x00000002

struct {
	enum cpu_vendor 	vendor;
	unsigned int		family;
	unsigned int		model;
	unsigned int		stepping;

	unsigned int 		capabilities;
} _cpu;


/*********************
 * Special registers *
 *********************/

/*
 * TSS
 */

struct tss {
	uint16_t	previous_task;
	uint16_t	reserved1;

	uint32_t	esp0;
	uint16_t	ss0;
	uint16_t	reserved2;
	uint32_t	esp1;
	uint16_t	ss1;
	uint16_t	reserved3;
	uint32_t	esp2;
	uint16_t	ss2;
	uint16_t	reserved4;

	uint32_t	cr3;
	uint32_t	eip;
	uint32_t	eflags;
	uint32_t	eax;
	uint32_t	ecx;
	uint32_t	edx;
	uint32_t	ebx;
	uint32_t	esp;
	uint32_t	ebp;
	uint32_t	esi;
	uint32_t	edi;

	uint16_t	es;
	uint16_t	reserved5;
	uint16_t	cs;
	uint16_t	reserved6;
	uint16_t	ss;
	uint16_t	reserved7;
	uint16_t	ds;
	uint16_t	reserved8;
	uint16_t	fs;
	uint16_t	reserved9;
	uint16_t	gs;
	uint16_t	reserved10;
	uint16_t	ldt;
	uint16_t	reserved11;
	uint16_t	debug_on_switch_flag;
	uint16_t	iomap_base;
} __attribute__((packed));

/*
 * Descriptor table flags
 */

/*** GDT ***/

#define CPU_GDT_ACCESS_PRESENT		0x4

/* Exclusive */
#define CPU_GDT_ACCESS_DPL_0		0x0
#define CPU_GDT_ACCESS_DPL_1		0x1
#define CPU_GDT_ACCESS_DPL_2		0x2
#define CPU_GDT_ACCESS_DPL_3		0x3

/* Exclusive */
#define CPU_GDT_TYPE_TASK		0x09

#define CPU_GDT_FLAG_AVAILABLE		0x1
#define CPU_GDT_FLAG_32BIT		0x4
#define CPU_GDT_FLAG_PAGE_LIMIT		0x8	/* Limit in page units */



/*** IDT ***/

#define CPU_IDT_ACCESS_PRESENT		0x4

/* Exclusive */
#define CPU_IDT_ACCESS_DPL_0		0x0
#define CPU_IDT_ACCESS_DPL_1		0x1
#define CPU_IDT_ACCESS_DPL_2		0x2
#define CPU_IDT_ACCESS_DPL_3		0x3

/* Exclusive */
#define CPU_IDT_TYPE_TASK		0x05
#define CPU_IDT_TYPE_INTERRUPT		0x0E	/* 32bit */
#define CPU_IDT_TYPE_TRAP		0x0F	/* 32bit */



/* 
 * This is a generic descriptor table entry.
 * It is suitable for GDTs, IDTs or can be accessed by 
 * a 64-bit value
 */

union dt_entry {
	
	/* Normal descriptor (e.g. GDT) */
	struct {
		uint16_t limit_low;     /* limit 0..15    */
		uint16_t base_low;      /* base  0..15    */
		uint8_t base_med;       /* base  16..23   */
		unsigned type:5;
		unsigned access:3;
		unsigned limit_high:4;  /* limit 16..19   */
		unsigned flags:4;   	/* flags          */
		uint8_t base_high;      /* base 24..31    */
	} __attribute__((packed)) desc;
	
	/* Gate descriptor (e.g. IDT) */
	struct {
		uint16_t offset_low;   /* offset 0..15    */
		uint16_t selector;     /* selector        */
		uint8_t	 reserved;
		unsigned type:5;
		unsigned access:3;     /* access flags    */
		uint16_t offset_high;  /* offset 16..31   */
	} __attribute__((packed)) gate;
	
	/* Dummy value */
	uint64_t dummy;
};

/* GDT descriptors. See x86.asm */
#define CPU_GDT_INDEX_NULL		0
#define CPU_GDT_INDEX_KERNEL_CS		1
#define CPU_GDT_INDEX_KERNEL_DS		2
#define CPU_GDT_INDEX_USER_CS		3
#define CPU_GDT_INDEX_USER_DS		4
#define CPU_GDT_INDEX_USER_SS		5
#define CPU_GDT_INDEX_TSS_DOUBLE_FAULT	6
#define CPU_GDT_INDEX_TSS_KERNEL	7

struct idt_info {
	uint16_t length;
	uint32_t addr;
} __attribute__((packed));

/* From x86.asm */
extern void cpu_mmu_invalidate(uint32_t start, size_t length);
extern void cpu_mmu_switch(uint32_t new_pgdir);

extern uint32_t cpu_flags_get(void);

err_t cpu_init(void);
void delay(unsigned int ms);


#endif /* !defined KERNEL_CPU_H */
