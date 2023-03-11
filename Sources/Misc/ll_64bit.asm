;
; Misc/ll_64bit.asm
; Written by The Neuromancer <neuromancer at paranoici dot org>
;
; This file is part of the Klesh operating system.
; Make sure you have read the license before copying, reading or
; modifying this document.
;
; Initial release: 2004-12-01
;
; Taken from SanOS, but heavily modified to support unsigned operations.
; TODO: comments not consistent.
;

SECTION .text

GLOBAL __umoddi3, __udivdi3


; ***********************
; * Unsigned operations *
; ***********************


__umoddi3:

        push    ebx

%define DVND_LO	[esp + 8]
%define DVND_HI	[esp + 8 + 4]
%define DVSR_LO	[esp + 16]
%define DVSR_HI	[esp + 16 + 4]


        mov     eax,DVSR_HI ; hi word of b
        or      eax,eax         ; test to see if signed
        jge     short .L2        ; skip rest if b is already positive
        mov     edx,DVSR_LO ; lo word of b
        neg     eax             ; make b positive
        neg     edx
        sbb     eax,0
        mov     DVSR_HI,eax ; save positive value
        mov     DVSR_LO,edx
.L2:

;
; Now do the divide.  First look to see if the divisor is less than 4194304K.
; If so, then we can use a simple algorithm with word divides, otherwise
; things get a little more complex.
;
; NOTE - eax currently contains the high order word of DVSR
;

        or      eax,eax         ; check to see if divisor < 4194304K
        jnz     short .L3        ; nope, gotta do this the hard way
        mov     ecx,DVSR_LO ; load divisor
        mov     eax,DVND_HI ; load high word of dividend
        xor     edx,edx
        div     ecx             ; edx <- remainder
        mov     eax,DVND_LO ; edx:eax <- remainder:lo word of dividend
        div     ecx             ; edx <- final remainder
        mov     eax,edx         ; edx:eax <- remainder
        xor     edx,edx
        jmp     short .L8        ; result sign ok, restore stack and return

;
; Here we do it the hard way.  Remember, eax contains the high word of DVSR
;

.L3:
        mov     ebx,eax         ; ebx:ecx <- divisor
        mov     ecx,DVSR_LO
        mov     edx,DVND_HI ; edx:eax <- dividend
        mov     eax,DVND_LO
.L5:
        shr     ebx,1           ; shift divisor right one bit
        rcr     ecx,1
        shr     edx,1           ; shift dividend right one bit
        rcr     eax,1
        or      ebx,ebx
        jnz     short .L5        ; loop until divisor < 4194304K
        div     ecx             ; now divide, ignore remainder

;
; We may be off by one, so to check, we will multiply the quotient
; by the divisor and check the result against the orignal dividend
; Note that we must also check for overflow, which can occur if the
; dividend is close to 2**64 and the quotient is off by 1.
;

        mov     ecx,eax         ; save a copy of quotient in ECX
        mul     dword  DVSR_HI
        xchg    ecx,eax         ; save product, get quotient in EAX
        mul     dword  DVSR_LO
        add     edx,ecx         ; EDX:EAX = QUOT * DVSR
        jc      short .L6        ; carry means Quotient is off by 1

;
; do long compare here between original dividend and the result of the
; multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
; subtract the original divisor from the result.
;

        cmp     edx,DVND_HI ; compare hi words of result and original
        ja      short .L6        ; if result > original, do subtract
        jb      short .L7        ; if result < original, we are ok
        cmp     eax,DVND_LO ; hi words are equal, compare lo words
        jbe     short .L7        ; if less or equal we are ok, else subtract
.L6:
        sub     eax,DVSR_LO ; subtract divisor from result
        sbb     edx,DVSR_HI
.L7:

;
; Calculate remainder by subtracting the result from the original dividend.
; Since the result is already in a register, we will do the subtract in the
; opposite direction and negate the result if necessary.
;

        sub     eax,DVND_LO ; subtract dividend from result
        sbb     edx,DVND_HI

;
; Just the cleanup left to do.  edx:eax contains the quotient.
; Restore the saved registers and return.
;

.L8:
        pop     ebx

        ret










__udivdi3:

        push    esi
        push    ebx

%define DVND_LO	[esp + 12]
%define DVND_HI	[esp + 12 + 4]
%define DVSR_LO	[esp + 20]
%define DVSR_HI	[esp + 20 + 4]

        mov     eax,DVSR_HI ; hi word of b
        or      eax,eax         ; test to see if signed
        jge     short .L2        ; skip rest if b is already positive
        inc     edi             ; complement the result sign flag
        mov     edx,DVSR_LO ; lo word of a
        neg     eax             ; make b positive
        neg     edx
        sbb     eax,0
        mov     DVSR_HI,eax ; save positive value
        mov     DVSR_LO,edx
.L2:

;
; Now do the divide.  First look to see if the divisor is less than 4194304K.
; If so, then we can use a simple algorithm with word divides, otherwise
; things get a little more complex.
;
; NOTE - eax currently contains the high order word of DVSR
;

        or      eax,eax         ; check to see if divisor < 4194304K
        jnz     short .L3        ; nope, gotta do this the hard way
        mov     ecx,DVSR_LO ; load divisor
        mov     eax,DVND_HI ; load high word of dividend
        xor     edx,edx
        div     ecx             ; eax <- high order bits of quotient
        mov     ebx,eax         ; save high bits of quotient
        mov     eax,DVND_LO ; edx:eax <- remainder:lo word of dividend
        div     ecx             ; eax <- low order bits of quotient
        mov     edx,ebx         ; edx:eax <- quotient
        jmp     short .L8        ; set sign, restore stack and return

;
; Here we do it the hard way.  Remember, eax contains the high word of DVSR
;

.L3:
        mov     ebx,eax         ; ebx:ecx <- divisor
        mov     ecx,DVSR_LO
        mov     edx,DVND_HI ; edx:eax <- dividend
        mov     eax,DVND_LO
.L5:
        shr     ebx,1           ; shift divisor right one bit
        rcr     ecx,1
        shr     edx,1           ; shift dividend right one bit
        rcr     eax,1
        or      ebx,ebx
        jnz     short .L5        ; loop until divisor < 4194304K
        div     ecx             ; now divide, ignore remainder
        mov     esi,eax         ; save quotient

;
; We may be off by one, so to check, we will multiply the quotient
; by the divisor and check the result against the orignal dividend
; Note that we must also check for overflow, which can occur if the
; dividend is close to 2**64 and the quotient is off by 1.
;

        mul     dword  DVSR_HI ; QUOT * DVSR_HI
        mov     ecx,eax
        mov     eax,DVSR_LO
        mul     esi             ; QUOT * DVSR_LO
        add     edx,ecx         ; EDX:EAX = QUOT * DVSR
        jc      short .L6        ; carry means Quotient is off by 1

;
; do long compare here between original dividend and the result of the
; multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
; subtract one (1) from the quotient.
;

        cmp     edx,DVND_HI ; compare hi words of result and original
        ja      short .L6        ; if result > original, do subtract
        jb      short .L7        ; if result < original, we are ok
        cmp     eax,DVND_LO ; hi words are equal, compare lo words
        jbe     short .L7        ; if less or equal we are ok, else subtract
.L6:
        dec     esi             ; subtract 1 from quotient
.L7:
        xor     edx,edx         ; edx:eax <- quotient
        mov     eax,esi

;
; Restore the saved registers and return.
;

.L8:
        pop     ebx
        pop     esi

        ret
