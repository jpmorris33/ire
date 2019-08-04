;
;	AMD64-ABI Assembler versions of the darkness routines
;
;	This will need modification to run in Windows because
;	they rolled their own ABI
;

[BITS 64]
[SECTION .text]

; Declare public symbols

GLOBAL _ire_checkMMX
GLOBAL ire_checkMMX

GLOBAL _darken_asm_32
GLOBAL darken_asm_32
GLOBAL _darken_asm_32s
GLOBAL darken_asm_32s
GLOBAL _darken_asm_16
GLOBAL darken_asm_16
GLOBAL _darken_asm_16s
GLOBAL darken_asm_16s
GLOBAL _darken_asm_blit16
GLOBAL darken_asm_blit16
GLOBAL _darken_asm_blit32
GLOBAL darken_asm_blit32
GLOBAL _darken_asm_darksprite
GLOBAL darken_asm_darksprite
GLOBAL _darken_asm_lightsprite
GLOBAL darken_asm_lightsprite

SMARTALIGN

_ire_checkMMX:
ire_checkMMX:
	push rbx
	mov rax,1	; Get feature flags
	cpuid
	mov rax,0	; Not there?
	test edx,23
	jz quitMMX
	mov rax,1	; It is there
quitMMX:
	pop rbx
	ret

	ALIGN 16

_darken_asm_32:
 darken_asm_32:

	; RDI = dest
	; RSI = src
	; RDX = len

	mov rcx,rdx		; We want it in CX, actually

	ALIGN 16

.darkloop:
	xor rax,rax
	xor rdx,rdx

	lodsb		   ; get source pixel
	mov edx,[rdi]      ; get the dest pixel

	; Replicate source level throughout EAX

	mov ah,al
	shl eax,8
	mov al,ah
	shl eax,8
	mov al,ah

	; Do the thing

	movd mm0,edx
	movd mm1,eax
	psubusb mm0,mm1
	movd eax,mm0
	stosd

	; Next pixel

	dec rcx
	jnz .darkloop

	ret

;;
;; Single colour (not using a darkmap)
;;

	ALIGN 16

_darken_asm_32s:
 darken_asm_32s:

	; RDI = dest
	; RSI = src (colour, not address!)
	; RDX = len

	mov rcx,rdx		; We want it in CX, actually

	; Do some pre-computation for the source colour level
	mov rax,rsi
	; Replicate source level throughout EAX
	mov ah,al
	shl eax,8
	mov al,ah
	shl eax,8
	mov al,ah
	and rax,0xffffff
	mov rsi,rax

	ALIGN 16

.darkloop:
	xor rdx,rdx
	mov rax,rsi
	mov edx,[rdi]      ; get the dest pixel
	; Do the thing

	movd mm0,edx
	movd mm1,eax
	psubusb mm0,mm1
	movd eax,mm0

	; We use colour separation in the roof projector, and 0 is transparent
	; So we need to make it non-zero unless it is supposed to be transparent

	test eax,eax ; Is it zero?
	jnz .darknoclip
	test edx,edx ; If it's meant to be 0, don't adjust it
	jz .darknoclip
	or eax,0x01000000
	ALIGN 16
.darknoclip:

	stosd			; Write output

	; Next pixel

	dec rcx
	jnz .darkloop

	ret

;;
;;	16bpp darkness code
;;

	ALIGN 16

_darken_asm_16:
darken_asm_16:

	push rbx

	; RDI = dest
	; RSI = src
	; RDX = len
	; RCX = LUT address

	; We want RBX as the LUT and RCX as the count
	mov rbx,rcx
	mov rcx,rdx

	; Now RCX = len
	; And RBX = LUT address

	; Clear high bits
	xor rdx,rdx
	xor rax,rax

	ALIGN 16

.darkloop:
	lodsb		   ; get source pixel in AL
	mov dx,[rdi]       ; get the dest pixel

	; Consult lookuptable to get correct lighting value
	; shl 13 instead of shl 16 converts light level to 5-bit, effective shr 3

	; ax = I32_clut[(ax*65536)+dx]
	and rax,0xf8					; ax = (ax)
	shl rax,13					; ax = (ax * 65536)
	add rax,rdx					; ax = (ax * 65536)+dx
	shl rax,1					; align to 16 bit array
	add rax,rbx					; I32_clut[(ax * 65536)+dx]
	mov ax,[rax]					; ax = I32_clut[(ax * 65536)+dx]
	stosw

	dec rcx
	jnz .darkloop

	pop rbx
	ret

;   Single colour (not using a darkmap)

	ALIGN 16

_darken_asm_16s:
darken_asm_16s:

	push rbx

	; RDI = dest
	; RSI = src colour, not address!
	; RDX = len
	; RCX = LUT address

	; We want RBX as the LUT and RCX as the count
	mov rbx,rcx
	mov rcx,rdx

	; Now RCX = len
	; And RBX = LUT address

	; Clear high bits
	xor rdx,rdx

	; Also trim the source colour, do some pre-computation
	and rsi,0xf8						; a = a
	shl rsi,13						; a = (a * 65536)
	; shl 13 instead of shl 16 converts light level to 5-bit, effective shr 3

	ALIGN 16

.darkloop:
	mov rax,rsi        ; get the source pixel
	mov dx,[rdi]       ; get the dest pixel

	; Consult lookuptable to get correct lighting value

	; ax = I32_clut[(ax*65536)+dx]
	add rax,rdx						; a = (a * 65536)+dx
	shl rax,1						; align to 16 bit array
	add rax,rbx						; I32_clut[(a * 65536)+d]
	mov dx,[rax]						; a = I32_clut[(a * 65536)+d]
	stosw

	dec rcx
	jnz .darkloop

	pop rbx
	ret

; Bitmap combining

; 16bpp

	ALIGN 16

_darken_asm_blit16:
 darken_asm_blit16:

	; RDI = dest
	; RSI = src colour, not address!
	; RDX = len

	mov rcx,rdx	; we want the length in CX

	ALIGN 16

.blitloop:
	lodsw
	test ax,ax
	jz .blitskip
	mov [rdi],ax
	ALIGN 16
.blitskip:
	add rdi,2
	dec rcx
	jnz .blitloop

	ret

; 32bpp

	ALIGN 16

_darken_asm_blit32:
 darken_asm_blit32:

	; RDI = dest
	; RSI = src colour, not address!
	; RDX = len

	mov rcx,rdx	; we want the length in CX
	ALIGN 16

.blitloop:
	lodsd
	test eax,eax
	jz .blitskip
	mov [rdi],eax
	ALIGN 16
.blitskip:
	add rdi,4
	dec rcx
	jnz .blitloop

	ret

; Darken Sprite (always 32x32)

	ALIGN 16

_darken_asm_darksprite:
 darken_asm_darksprite:

	; RDI = dest
	; RSI = src
	; RDX = LUT address, not used for MMX
	; RCX = offset

	mov rdx,rcx		; Set this to be the width offset to next line
	mov rcx,32		; 32 lines

	ALIGN 16

darkspriteloop:
	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	paddusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	paddusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	paddusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	paddusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	add rdi, rdx
	dec rcx
	jnz darkspriteloop

	ret

; Lighten Sprite (always 32x32)

	ALIGN 16

_darken_asm_lightsprite:
 darken_asm_lightsprite:

	; RDI = dest
	; RSI = src
	; RDX = LUT address, not used for MMX
	; RCX = offset

	mov rdx,rcx		; Set this to be the width offset to next line
	mov rcx,32		; 32 lines

	ALIGN 16

lightspriteloop:
	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	psubusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	psubusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	psubusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	movq mm0,[rdi]       ; get the dest pixels
	movq mm1,[rsi]       ; get the source pixels
	psubusb mm0,mm1
	movq [rdi],mm0		; Write output
	add rdi,8
	add rsi,8

	add rdi, rdx
	
	dec rcx
	jnz lightspriteloop

	ret
