;
; start.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Nehemiah operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2004-11-23 
;

; From x86.asm
EXTERN _gdt_info, _initial_stack

; From main.c
EXTERN main

GLOBAL _nehemiah, _freeze

; Multiboot loader block
multiboot_magic	equ	0x1badb002
multiboot_flags	equ	0x00000003

ALIGN 4
	dd	multiboot_magic
	dd	multiboot_flags
	dd	0-(multiboot_magic+multiboot_flags)

_nehemiah:
	cld

	; Load a new GDT and new selectors
	lgdt	[_gdt_info]

	mov	dx, 0x10
	mov	ds, dx
	mov	es, dx
	mov	fs, dx
	mov	gs, dx

	jmp	0x08:.new_gdt

.new_gdt:
	; Set the initial kernel stack
	mov	esp, _initial_stack

	; Save the multiboot info structure address and
	; pass to main() the multiboot loader magic
	mov	[_multiboot], ebx
	push	eax

	call	main

_freeze:
	hlt
	jmp	short _freeze



SECTION .bss

COMMON  _multiboot 4
