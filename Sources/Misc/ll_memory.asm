;
; Misc/ll_memory.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Klesh operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2004-11-23 
;

GLOBAL memory_fill, memory_clear, memory_copy, memory_swap_word, 

SECTION .text

ALIGN 16
memory_fill:
	push	edi

	mov	edi, [esp + 8]
	mov	eax, [esp + 12]
	mov	ecx, [esp + 16]

	rep	stosd

	pop	edi

	ret
	
ALIGN 16
memory_clear:
	push	edi

	mov	edi, [esp + 8]
	mov	ecx, [esp + 12]
	
	xor	eax, eax
	
	test	cl, 1
	jz	.test_word
	
	mov	[edi], al
	inc	edi

.test_word:
	test	cl, 2
	jz	.test_dword

	push	ecx
	
	shr	ecx, 1
	mov	[edi], ax
	add	edi, 2

	pop	ecx
	
.test_dword:	
	shr	ecx, 2
	rep	stosd
	
	pop	edi

	ret
	
ALIGN 16
memory_copy:
	push	edi
	push	esi

	mov	edi, [esp + 12]
	mov	esi, [esp + 16]
	mov	ecx, [esp + 20]
	push	ecx
	
	cmp	edi, esi
	jl	.normal
	
.reverse:
	std

	lea	edi, [edi + ecx - 1]
	lea	esi, [esi + ecx - 1]
	
	and	ecx, 3
	rep	movsb

	sub	edi, 3
	sub	esi, 3	
	pop	ecx
	shr	ecx, 2
	rep	movsd		
	
	cld
	jmp	short .end
	
.normal:
	and	ecx, 3
	rep	movsb

	pop	ecx	
	shr	ecx, 2
	rep	movsd

.end:
	pop	esi
	pop	edi

	ret
	
ALIGN 16
memory_swap_word:
	mov	edx, [esp + 4]
	mov	ax, [edx]
	
	rol	ax, 8
	
	mov	[edx], ax
	ret
