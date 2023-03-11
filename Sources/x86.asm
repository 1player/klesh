;
; x86.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Klesh operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2004-11-23 
;

; ***************************
; * Global Descriptor Table *
; ***************************

GLOBAL _gdt, _gdt_info

SECTION .data

_gdt:
; 0x00 - null selector
	dw	0
	dw	0
	db	0
	db	0
	db	0
	db	0

; 0x08 - kernel code selector
	dw	0xFFFF
	dw	0
	db	0
	db	0x9A
	db	0xCF
	db	0

; 0x10 - kernel data selector
	dw	0xFFFF
	dw	0
	db	0
	db	0x92
	db	0xCF
	db	0

; 0x18 - user code selector
	dw	0xFFFF
	dw	0
	db	0
	db	0xFA
	db	0xCF
	db	0

; 0x20 - user data selector
	dw	0xFFFF
	dw	0
	db	0
	db	0xF2
	db	0xCF
	db	0

; 0x28 - user stack selector
	dw	0
	dw	0
	db	0
	db	0
	db	0
	db	0

; 0x30 - double fault TSS. Set in process/init.c
	dw	0
	dw	0
	db	0
	db	0
	db	0
	db	0

; 0x38 - kernel TSS used to store the kernel ESP
	dw	0
	dw	0
	db	0
	db	0
	db	0
	db	0
_gdt_end:

_gdt_info:
	dw	_gdt_end - _gdt - 1
	dd	_gdt


; *********
; * TSSes *
; *********

SECTION .bss

COMMON _double_fault_tss 104
COMMON _kernel_tss	 104


; *******
; * CPU *
; *******

GLOBAL cpu_cpuid_supported, cpu_cpuid_call, cpu_rdtsc, cpu_usermode

SECTION .init

; CPU identification
; check is CPUID instruction present 
; (can change bit 21 in EFLAGS - ID)
cpu_cpuid_supported:
	push	ebx

	pushfd
	pop 	edx		; current EFLAGS content
	mov 	ebx, edx
	xor 	edx, 1 << 21	; invert bit 21
	push 	edx
	popfd			; try to change bit 21
	pushfd
	pop 	edx

	xor	eax, eax
	cmp 	edx, ebx	; changed?
	setne	al

	pop	ebx
	ret

SECTION .text

cpu_cpuid_call:
	push	esi

	mov	eax, [esp + 8]

	cpuid

	mov	esi, [esp + 12]
	mov	[esi], eax

	mov	esi, [esp + 16]
	mov	[esi], ebx

	mov	esi, [esp + 20]
	mov	[esi], ecx

	mov	esi, [esp + 24]
	mov	[esi], edx

	pop	esi
	ret	4 * 5
	
cpu_rdtsc:
	rdtsc
	ret
	
cpu_usermode:
	cli
	; Go to ring3 from ring0
	;add	esp, 4		; The first argument is the EIP
	push	0x20 + 3	; User SS
	push	0x0		; User ESP
	push	0x200		; EFlags
	push	0x18 + 3	; User CS
	push	0xC0000000	; User EIP
	
	; Load segment registers
	mov 	ax, 0x20 + 3
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	
	iret



; ******************
; * Paging and MMU *
; ******************

SECTION .init

GLOBAL cpu_paging_enable

cpu_paging_enable:
	mov	eax, [esp + 4]
	mov	cr3, eax

	mov	eax, cr0
	or	eax, 0x80000000		; Enable paging flag
	mov	cr0, eax

	jmp	.1

.1:
	ret

SECTION .text

GLOBAL cpu_mmu_invalidate, cpu_mmu_switch

cpu_mmu_invalidate:
	mov	eax, [esp + 4]
	mov	ecx, [esp + 8]

	shl	eax, 12

.next:
	invlpg	[eax]
	add	eax, 0x1000
	loop	.next

	ret

cpu_mmu_switch:
	mov	eax, [esp + 4]
	mov	cr3, eax

	jmp	.1
.1:
	ret

; **************
; * Interrupts *
; **************

SECTION .text

GLOBAL interrupt_lidt

interrupt_lidt:
	mov	eax, [esp + 4]
	lidt	[eax]

	jmp	.1

.1:
	ret



SECTION .text

; From interrupt.c
EXTERN interrupt_trap_exception, interrupt_trap_irq, process_schedule

; From timer.h
EXTERN _ticks

%macro INT_HANDLER		1
GLOBAL _int%1_handler

_int%1_handler:
	cld

	push	gs
	push	fs
	push	es
	push	ds
	pusha

	push	dword [esp + 48]	; EIP
	push	dword [esp + 56]	; selector
	push	dword 0			; address
	push	dword 0			; error code
	push	dword %1		; interrupt number
	
	call	interrupt_trap_exception
	
	add	esp, 4 * 5

	popa
	pop	ds
	pop	es
	pop	fs
	pop	gs

	iret
%endmacro

%macro INT_HANDLER_ERROR	1
GLOBAL _int%1_handler

_int%1_handler:
	cld

	push	gs
	push	fs
	push	es
	push	ds
	pusha

	push	dword [esp + 52]	; EIP
	push	dword [esp + 60]	; selector
	push	dword 0			; address
	push	dword [esp + 60]	; error code
	push	dword %1		; interrupt number
	
	call	interrupt_trap_exception
	
	add	esp, 4 * 5

	popa
	pop	ds
	pop	es
	pop	fs
	pop	gs

	add 	esp, 4		; skip error code
	iret
%endmacro

%macro INT_HANDLER_PAGE		1
GLOBAL _int%1_handler

_int%1_handler:
	cld

	push	gs
	push	fs
	push	es
	push	ds
	pusha

	push	dword [esp + 52]	; EIP
	push	dword [esp + 60]	; selector
	mov	eax, cr2
	push	eax			; address
	push	dword [esp + 60]	; error code
	push	dword %1		; interrupt number
	
	call	interrupt_trap_exception
	
	add	esp, 4 * 5

	popa
	pop	ds
	pop	es
	pop	fs
	pop	gs

	add 	esp, 4		; skip error code
	iret
%endmacro

%macro IRQ_HANDLER	1
GLOBAL _irq%1_handler

_irq%1_handler:
	cld

	pusha
	
	mov	ax, 0x10
	mov	ds, ax
	mov	es, ax

	push	dword	%1
	call	interrupt_trap_irq
	add	esp, 4

	; EOI
	mov	al, 0x20
	out	0x20, al
	out	0xA0, al

	popa

	iret
%endmacro

INT_HANDLER		0
INT_HANDLER		1
INT_HANDLER		2
INT_HANDLER		3
INT_HANDLER		4
INT_HANDLER		5
INT_HANDLER		6
INT_HANDLER		7
INT_HANDLER_ERROR 	10
INT_HANDLER_ERROR 	11
INT_HANDLER_ERROR 	12
INT_HANDLER_ERROR 	13
INT_HANDLER_PAGE	14
INT_HANDLER		16
INT_HANDLER_ERROR 	17
INT_HANDLER		18

IRQ_HANDLER		1
IRQ_HANDLER		2
IRQ_HANDLER		3
IRQ_HANDLER		4
IRQ_HANDLER		5
IRQ_HANDLER		6
IRQ_HANDLER		7
IRQ_HANDLER		8
IRQ_HANDLER		9
IRQ_HANDLER		10
IRQ_HANDLER		11
IRQ_HANDLER		12
IRQ_HANDLER		13
IRQ_HANDLER		14
IRQ_HANDLER		15

GLOBAL _irq0_handler, process_thread_reschedule
EXTERN current_thread
_irq0_handler:
	cld

	pusha

	; Increment ticks
	inc	dword [_ticks]

	call	process_schedule

	; Save the old thread's stack in its ESP variable
	mov	[eax + 0], esp

	; Load the new thread's stack
	mov	edi, [current_thread]
	mov	esp, [edi + 0]

	; EOI
	mov	al, 0x20
	out	0x20, al

	popa
	iret


process_thread_reschedule:
	; Simulate a timer interrupt
	mov	eax, [esp]	; Store the EIP
	add	esp, 4		; Use the flags which are the first argument
	;pushf			; Save the flags
	push	0x08		; Save the selector
	push	eax		; Save the EIP

	pusha

	call	process_schedule

	; Save the old thread's stack in its ESP variable
	mov	[eax + 0], esp

	; Load the new thread's stack
	mov	edi, [current_thread]
	mov	esp, [edi + 0]

	popa
	iret

	

; Generic handlers

GLOBAL _int_unhandled

_int_unhandled:
	cld
	
	mov	ax, 0x10
	mov	ss, ax

	push	gs
	push	fs
	push	es
	push	ds
	pusha

	push	dword [esp + 48]	; EIP
	push	dword [esp + 56]	; selector
	push	dword 0			; address
	push	dword 0			; error code
	push	dword -1		; interrupt number
	
	call	interrupt_trap_exception
	add	esp, 4 * 5

	popa
	pop	ds
	pop	es
	pop	fs
	pop	gs

	iret


GLOBAL interrupt_enable, interrupt_disable

interrupt_enable:
	sti
	ret

interrupt_disable:
	cli
	ret

GLOBAL cpu_flags_get
cpu_flags_get:
	pushf
	pop	eax
	ret
	
GLOBAL _syscall_misc_trap
EXTERN syscall_misc
_syscall_misc_trap:
	push	ebx
	push	ecx
	push	esi
	push	edi
	push	ebp
	
	push	ds
	push	es
	
	; Load kernel data descriptors
	mov	bx, 0x10
	mov	ds, bx
	mov	es, bx
	
	push	edx		; Syscall arg
	push	eax		; Syscall number
	call	syscall_misc
	add	esp, 4 * 2
	
	pop	es
	pop	ds
	
	pop	ebp
	pop	edi
	pop	esi
	pop	ecx
	pop	ebx
	
	iret

; **********************
; * Process management *
; **********************

SECTION .text

GLOBAL process_ltr

process_ltr:
	ltr	[esp + 4]

	jmp	.1
.1:
	ret


; **************************************************
; * Uninitialized page-aligned data and structures *
; **************************************************

; Make sure that each data length is a multiple of 4096

section .page_aligned nobits alloc noexec write align=4096

GLOBAL _initial_stack, _process_page_directory, _dummy_page_directory


	resd	4096
_initial_stack:

; Those pages will be immediately freed and put on the physical page stack.
; It is only a placeholder where we will map the process page directories we are going to modify
_process_page_directory:
	resd	1024
	
; It is only a placeholder where we will map the page directories we are going to modify
_dummy_page_directory:
	resd	1024
