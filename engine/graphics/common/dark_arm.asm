;
;	ARMv7 versions of the darkness routines
;

        format ELF
;        entry start
;        segment readable executable

USE32

section '.text'
; code readable executable

; Declare public symbols

public _ire_checkMMX
public ire_checkMMX

public _darken_asm_32
public darken_asm_32
public _darken_asm_32s
public darken_asm_32s
public _darken_asm_16
public darken_asm_16
public _darken_asm_16s
public darken_asm_16s
public _darken_asm_blit16
public darken_asm_blit16
public _darken_asm_blit32
public darken_asm_blit32
public _darken_asm_darksprite
public darken_asm_darksprite
public _darken_asm_lightsprite
public darken_asm_lightsprite


;;
;;  Check for NEON
;;

_ire_checkMMX:
 ire_checkMMX:
	mov r0,1	; ARM won't let us find out if NEON is there or not.  Clever.
	mov pc,lr


;;
;; 32bpp darken
;;

_darken_asm_32:
 darken_asm_32:

	; r0 = dest
	; r1 = src address
	; r2 = len

	push {r4}
	push {r5}

darkloop32:
;	ldmiab r1!,{r3}	; Get src byte (in R3) and increment
	ldrb r3,[r1]	; Get src byte (in R3)
	add r1,#1
	ldmia r0,{r4}	; Get dest colour (in R4), don't increment

	; Replicate source level throughout R3

;	mov r5,r3 lsl #8 	; copy to r5, shifted one byte
	mov r5,r3
	lsl r5, #8 		; shift by one byte

	orr r3,r5		; merge r5 into r3 so there are two

;	mov r5,r3 lsl #16 	; Now replicate into top half
	mov r5,r3
	lsl r5, #16 		; shift by one byte

	orr r3,r5

	; Subtract parallel bytes with saturation, MMX-style
	uqsub8 r4, r4, r3	; r4=r4-r3

;	stria r0!,[r4]		; Write back and increment
	stmia r0!,{r4}		; Write back and increment

	subs r2,#1	; subtract and set flags for loop
	bne darkloop32

	pop {r5}
	pop {r4}
	mov pc,lr



; Single colour (not using a darkmap)

_darken_asm_32s:
 darken_asm_32s:

	; r0 = dest
	; r1 = src colour (not address!)
	; r2 = len

	push {r4}

	; Replicate source pixel out to 32 bits

	mov r3,r1
	lsl r3, #8 		; shift by one byte
	orr r1,r3		; merge r5 into r3 so there are two
	mov r3,r1
	lsl r3, #16 		; shift by one byte
	orr r1,r3

	; R1 now contains the complete colour data

darkloop32s:

	ldmia r0,{r4}	; Get dest colour (in R4), don't increment

	; Back it up into R3
	mov r3,r4

	; Subtract parallel bytes with saturation, MMX-style
	uqsub8 r3, r3, r1	; r3=r3-r1

	; We use colour separation in the roof projector, and 0 is transparent
	; So we need to make it non-zero unless it is supposed to be transparent

	cmp r3,#0 ; is it zero?
	bne dark32noclip	; No, forget it

	; Yes.  Was it before?
	cmp r4,#0
	beq dark32noclip	; Yes, it is supposed to be 0, don't adjust it
	orr r3,#01000000h	; Set a bit to make it nonzero
dark32noclip:

	stmia r0!,{r3}		; Write back and increment

	subs r2,#1	; subtract and set flags for loop
	bne darkloop32s

	pop {r4}
	mov pc,lr


;;
;;	16bpp darkness code
;;

_darken_asm_16:
darken_asm_16:

	; r0 = dest
	; r1 = src address
	; r2 = len
	; r3 = LUT address

	push {r4-r6}


darkloop16:
	ldrh r5,[r0]	; Get dest word pixel (in R5)
	ldrb r4,[r1]	; Get src byte (in R4)
	add r1,#1

	; Consult lookuptable to get correct lighting value
	; shl 13 instead of shl 16 converts light level to 5-bit, effective shr 3

	; r4 = I32_clut[(r4*65536)+r5]
	and r4,#0f8h						; r6 = (r4)
	lsl r4,#13						; r6 = (r4 * 65536)
	add r4,r5						; r6 = (r4 * 65536)+r5
	lsl r4,#1						; align to 16 bit array
	add r4,r3						; I32_clut[(r4 * 65536)+r5]
	ldrh r6,[r4]						; r6 = I32_clut[(r4 * 65536)+r5]

	; Write it back and increment
	strh r6,[r0]
	add r0,#2

	subs r2,#1	; subtract and set flags for loop
	bne darkloop16

	pop {r4-r6}
	mov pc,lr

;;
;;  Single colour (not using a darkmap)
;;

_darken_asm_16s:
darken_asm_16s:

	; r0 = dest
	; r1 = src colour (not address!)
	; r2 = len
	; r3 = LUT address

	push {r4-r6}

darkloop16s:
	mov r4,r1
	ldrh r5,[r0]	; Get dest word pixel (in R4)

	; Consult lookuptable to get correct lighting value
	; shl 13 instead of shl 16 converts light level to 5-bit, effective shr 3

	; r4 = I32_clut[(r4*65536)+r5]
	and r4,#0f8h						; r6 = (r4)
	lsl r4,#13						; r6 = (r4 * 65536)
	add r4,r5						; r6 = (r4 * 65536)+r1
	lsl r4,#1						; align to 16 bit array
	add r4,r3						; I32_clut[(r4 * 65536)+r1]
	ldrh r5,[r4]						; r5 = I32_clut[(r4 * 65536)+r1]

	; Write it back and increment
	strh r5,[r0]
	add r0,#2

	subs r2,#1	; subtract and set flags for loop
	bne darkloop16s

	pop {r4-r6}
	mov pc,lr


;;
;; 16bpp blit
;;

_darken_asm_blit16:
darken_asm_blit16:
;	r0 = dest, r1 = src, r2 = len

blitloop16:
	ldrh r3,[r1]	; Get src colour (in R3)
	tst r3,#0	; Is it zero?
	strhne r0,[r3]	; Store R3 into R0 if R0 is nonzero

	add r0,#2	; Increment R0 whether it was or was not written

	subs r2,#1	; subtract and set flags
	bne blitloop16

	mov pc,lr	; return

;;
;; 32bpp blit
;;

_darken_asm_blit32:
darken_asm_blit32:
;	r0 = dest, r1 = src, r2 = len

blitloop32:
	ldmia r1!,{r3}	; Get src colour (in R3)
	tst r3,#0	; Is it zero?
	strne r0,[r3]	; Store R3 into R0 if R0 is nonzero

	add r0,#4	; Increment R0 whether it was or was not written

	subs r2,#1	; subtract and set flags
	bne blitloop32

	mov pc,lr	; return

;end

;;
;;	Darken sprite
;;

_darken_asm_darksprite:
darken_asm_darksprite:
;	r0 = dest, r1 = src, r2 = LUT offset, r3 = offset to next line

	mov r2,#32	; 32 lines
darkspriteloop:
	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqadd.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1

	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqadd.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1

	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqadd.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1

	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqadd.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1

	add r0,r3	; Add offset

	subs r2,#1	; subtract and set flags
	bne darkspriteloop
	mov pc,lr	; return

;end

;;
;;	Light sprite
;;

_darken_asm_lightsprite:
darken_asm_lightsprite:
;	r0 = dest, r1 = src, r2 = LUT offset, r3 = offset to next line

	mov r2,#32	; 32 lines
lightspriteloop:
	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqsub.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1
	
	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqsub.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1
	
	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqsub.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1
	
	vldmia r0!,{d2}
	vldmia r1!,{d3}
	vqsub.U8 D2,D3,D2
	vstmia r0,{d2}
	add r0,#8	; Increment R0
	add r1,#8	; Increment R1
	
	add r0,r3	; Add offset

	subs r2,#1	; subtract and set flags
	bne lightspriteloop
	mov pc,lr	; return

;end
