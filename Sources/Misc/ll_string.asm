;
; ll_string.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Nehemiah operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2004-11-24 
;

GLOBAL string_length

SECTION .text

string_length:
	push	edi

	mov	edi, [esp + 8]
	mov	ecx, 0xFFFFFFFF
	xor	eax, eax
	
	repne	scasb
	not	ecx
	dec	ecx
	
	mov	eax, ecx

	pop	edi

	ret
