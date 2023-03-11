;
; Misc/ll_bit.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Klesh operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2005-05-13
;

GLOBAL bit_find_set, bit_find_reset, bit_set, bit_reset, bit_invert, bit_test

ALIGN 16
bit_find_set:
	bsf	eax, [esp + 4]
	ret

ALIGN 16
bit_find_reset:
	mov	edx, [esp + 4]
	not	edx
	bsf	eax, edx
	ret

ALIGN 16
bit_set:
	mov	eax, [esp + 4]
	mov	edx, [esp + 8]
	bts	eax, edx
	ret

ALIGN 16
bit_reset:
	mov	eax, [esp + 4]
	mov	edx, [esp + 8]
	btr	eax, edx
	ret

ALIGN 16
bit_invert:
	mov	eax, [esp + 4]
	mov	edx, [esp + 8]
	btc	eax, edx
	ret	

ALIGN 16
bit_test:
	xor	eax, eax
	mov	edx, [esp + 8]
	bt	[esp + 4], edx
	setc	al
	ret
