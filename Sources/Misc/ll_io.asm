;
; Misc/io.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Klesh operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2004-11-23 
;

GLOBAL port_read_byte, port_write_byte, port_read_word

SECTION .text

ALIGN 16
port_read_byte:
	push	ebx

	mov	edx, [esp + 8]
	mov	ebx, [esp + 12]
	
	in	al, dx
	mov	[ebx], al

	pop	ebx
	
	ret
	
ALIGN 16
port_write_byte:

	mov	edx, [esp + 4]
	mov	eax, [esp + 8]
	
	out	dx, al

	ret
	
ALIGN 16
port_read_word:
	push	ebx

	mov	edx, [esp + 8]
	mov	ebx, [esp + 12]
	
	in	ax, dx
	mov	[ebx], ax

	pop	ebx
	
	ret
