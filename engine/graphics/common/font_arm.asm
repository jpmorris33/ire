;
;	ARMv7 versions of the font routines
;

        format ELF
;        entry start
;        segment readable executable

USE32

section '.text'
; code readable executable

; Declare public symbols

; Declare public symbols

public _draw_bitfont_32asm
public draw_bitfont_32asm
public _draw_bitfont_16asm
public draw_bitfont_16asm
public _draw_bytefont_32asm
public draw_bytefont_32asm
public _draw_bytefont_16asm
public draw_bytefont_16asm
public _draw_colourfont_32asm
public draw_colourfont_32asm
public _draw_colourfont_16asm
public draw_colourfont_16asm

;   Monochrome bitpacked fonts

; 32bpp

_draw_bitfont_32asm:
draw_bitfont_32asm:

	push {r4-r8}

	;	r0 = dest
	;	r1 = source
	;	r2 = width offset
	;	r3 = colour

	; Get width offset and convert to dwords, stick into r5
	lsl r5,r2,#2
	mov r4,r0

	eor r0,r0	; Clear high words
	mov r6,#8	; 8 pixels high
	
	; Now r0 = scratch, r1 = source, r2 = width, r3 = colour, r4 = dest, r5 = width offset, r6 = height

bitfont32h:

	mov r2,#8	; Set to 8 pixels wide
	
	ldrb r0,[r1] ; get byte into r0
	add r1,#1
	
bitfont32w:
	tst r0,#128	; Test high bit of AL
	stmiane r4,{r3}	; Write and don't increment
	add r4,#4
	lsl r0,#1	; Slide AL across to get next bit
	subs r2,#1
	bne bitfont32w

	add r4, r5
	subs r6,#1
	bne bitfont32h

	pop {r4-r8}
	mov pc,lr

; 16bpp

_draw_bitfont_16asm:
draw_bitfont_16asm:
	push {r4-r8}

	;	r0 = dest
	;	r1 = source
	;	r2 = width offset
	;	r3 = colour

	; Get width offset and convert to words, stick into r5
	lsl r5,r2,#1
	mov r4,r0

	eor r0,r0	; Clear high words
	mov r6,#8	; 8 pixels high
	
	; Now r0 = scratch, r1 = source, r2 = width, r3 = colour, r4 = dest, r5 = width offset, r6 = height

	mov r2,#8	; Set to 8 pixels wide
	
	ldrb r0,[r1],#1 ; get byte into r0
;	add r1,#1

	ALIGN 4
	
bitfont16w:
	tst r0,#128	; Test high bit of AL
	strhne r3,[r4]	; Write and don't increment
	add r4,#2
	lsl.b r0,#1	; Slide AL across to get next bit
	subs r2,#1
	bne bitfont16w

	add r4, r5
	subs r6,#1
	bne bitfont16h

	pop {r4-r8}
	mov pc,lr

;   Monochrome sprite fonts

; 32bpp

_draw_bytefont_32asm:
draw_bytefont_32asm:
	mov pc,lr

	;	r0 = dest
	;	r1 = font data
	;	r2 = width
	;	r3 = height

	mov r12,sp
	push {r4-r8}
	stmfd r12,{r4-r5}	; r4 = offset, r5 = colour

	; Convert width offset to dwords
	lsl r4,#2

	; Move dest to R6, R0 as scratch
	mov r6,r0
	eor r0,r0	; Clear high words
	
	; Now r0 = scratch, r1 = source, r2 = width, r3 = height, r4 =offset, r5 = colour, r6 = dest

bytefont32h:

	mov r7,r2		;width
	
bytefont32w:
	
	ldrb r0,[r1],#1 ; get byte into r0 and increment r1
	tst r0,r0
	stmiaeq r6,{r4}	; Write and don't increment
bytefont32skip:
	add r6,#4
	subs r7,#1
	bne bytefont32w

	add r6,r4
	subs r3,#1
	bne bytefont32h

	pop {r4-r8}
	mov pc,lr

; 16bpp

_draw_bytefont_16asm:
draw_bytefont_16asm:
	mov pc,lr

	;	r0 = dest
	;	r1 = font data
	;	r2 = width
	;	r3 = height

	mov r12,sp
	push {r4-r7}
	stmfd r12,{r4-r5}	; r4 = offset, r5 = colour

	; Convert width offset to words
	lsl r4,#1

	; Move dest to R6, R0 as scratch
	mov r6,r0
	eor r0,r0	; Clear high words
	
	; Now r0 = scratch, r1 = source, r2 = width, r3 = height, r4 =offset, r5 = colour, r6 = dest

bytefont16h:

	mov r7,r2		;width
	
bytefont16w:
	
	ldrb r0,[r1],#1 ; get byte into r0 and increment r1
	tst r0,r0
	strheq r5,[r6]	; Write and don't increment
bytefont16skip:
	add r6,#4
	subs r7,#1
	bne bytefont16w

	add r6,r4
	subs r3,#1
	bne bytefont16h

	pop {r4-r7}
	mov pc,lr


;   Colour sprite fonts

; 32bpp

_draw_colourfont_32asm:
draw_colourfont_32asm:
	;	r0 = dest
	;	r1 = font data
	;	r2 = palette
	;	r3 = width

	mov r12,sp
	push {r4-r8}
	stmfd r12,{r4-r5}	; r4 = height, r5 = offset

	; Convert width offset to dwords
	lsl r4,#2

	; Move dest to R6, R0 as scratch
	mov r6,r0
	eor r0,r0	; Clear high words
	
	; Now r0 = scratch, r1 = source, r2 = palette, r3 = width, r4 =height, r5 = offset, r6 = dest

colourfont32h:

	mov r7,r3		;width
	
colourfont32w:
	
	ldrb r0,[r1],#1 ; get byte into r0 and increment r1
	tst r0,r0
	beq  colourfont32skip
	lsl r0,#2		; Multiply R0 by 4 to convert to dword index
	ldr r8,[r2,r0]	; Lookup
	str r8,[r6]	; Write to dest
	eor r0,r0		; Clear it
colourfont32skip:
	add r6,#4
	subs r7,#1
	bne colourfont32w

	add r6, r5
	subs r4,#1
	bne colourfont32h

	pop {r4-r8}
	mov pc,lr

_draw_colourfont_16asm:
draw_colourfont_16asm:
	;	r0 = dest
	;	r1 = font data
	;	r2 = palette
	;	r3 = width

	mov r12,sp
	push {r4-r8}
	stmfd r12,{r4-r5}	; r4 = height, r5 = offset

	; Convert width offset to words
	lsl r4,#1

	; Move dest to R6, R0 as scratch
	mov r6,r0
	eor r0,r0	; Clear high words
	
	; Now r0 = scratch, r1 = source, r2 = palette, r3 = width, r4 =height, r5 = offset, r6 = dest

colourfont16h:

	mov r7,r3		;width
	
colourfont16w:
	
	ldrb r0,[r1],#1 ; get byte into r0 and increment r1
	tst r0,r0
	beq  colourfont16skip
	lsl r0,#2		; Multiply R0 by 4 to convert to dword index (yes, even in 16bpp)
	ldr r8,[r2,r0]	; Lookup
	strh r8,[r6]	; Write to dest
	eor r0,r0		; Clear it
colourfont16skip:
	add r6,#2
	subs r7,#1
	bne colourfont16w

	add r6, r5
	subs r4,#1
	bne colourfont16h

	pop {r4-r8}
	mov pc,lr

