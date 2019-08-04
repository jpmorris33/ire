;
;	i386 Assembler versions of the darkness routines
;

[BITS 32]
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


_ire_checkMMX:
ire_checkMMX:
	push ebx
	mov eax,1	; Get feature flags
	cpuid
	
	mov eax,0	; Not there?
	test edx,23
	jz quitMMX
	mov eax,1	; It is there
quitMMX:
	
	pop ebx
	ret



;	MMX darkness code

	ALIGN 4

_darken_asm_32:
 darken_asm_32:
	push ebp
	mov ebp,esp

	push edi
	push esi
	emms

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	mov ecx,[ebp+16]		;Len

	ALIGN 4
	
.darkloop:
	lodsb			; Get source pixel into AL
	mov edx,[edi]      ; get the dest pixel

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
	dec ecx
	jnz .darkloop

	emms
	pop esi
	pop edi
	pop ebp
	ret

; Single colour (not using a darkmap)

	ALIGN 4

_darken_asm_32s:
 darken_asm_32s:
	push ebp
	mov ebp,esp

	push edi
	push esi

	emms

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source colour (not address)
	mov ecx,[ebp+16]		;Len

	ALIGN 4

.darkloop:
	mov eax,esi       ; get the source pixel
	mov edx,[edi]      ; get the dest pixel

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

	; We use colour separation in the roof projector, and 0 is transparent
	; So we need to make it non-zero unless it is supposed to be transparent

	test eax,eax ; Is it zero?
	jnz .darknoclip
	test edx,edx ; If it's meant to be 0, don't adjust it
	jz .darknoclip
	or eax,0x01000000
	ALIGN 4
.darknoclip:

	stosd	; Write output

	; Next pixel
	dec ecx
	jnz .darkloop

	pop esi
	pop edi
	pop ebp
	ret

;	16bpp darkness code

	ALIGN 4

_darken_asm_16:
darken_asm_16:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx

	mov edi,[ebp+8]	        ;Dest
	mov esi,[ebp+12]		;Source
	mov ecx,[ebp+16]        ;Len
	mov ebx,[ebp+20]        ;LUT address

	xor edx,edx          ; Clear high bits

.darkloop:
	lodsb		   ; get source pixel into AL
	mov dx,[edi]       ; get the dest pixel

	; Consult lookuptable to get correct lighting value
	; shl 13 instead of shl 16 converts light level to 5-bit, effective shr 3

	; ax = I32_clut[(ax*65536)+dx]
	and eax,0x000000f8				; ax = (ax)
	shl eax,13						; ax = (ax * 65536)
	add eax,edx						; ax = (ax * 65536)+dx
	shl eax,1						; align to 16 bit array
	add eax,ebx						; I32_clut[(ax * 65536)+dx]
	mov ax,[eax]					; ax = I32_clut[(ax * 65536)+dx]

	stosw
	dec ecx
	jnz .darkloop

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

;   Single colour (not using a darkmap)

	ALIGN 4

_darken_asm_16s:
darken_asm_16s:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx

	mov edi,[ebp+8]	        ;Dest
	mov esi,[ebp+12]		;Source colour (not address)
	mov ecx,[ebp+16]        ;Len
	mov ebx,[ebp+20]        ;LUT address

	xor edx,edx          ; Clear high bits

	ALIGN 4

.darkloop:
	mov eax,esi        ; get the source pixel
	mov dx,[edi]       ; get the dest pixel

	; Consult lookuptable to get correct lighting value
	; shl 13 instead of shl 16 converts light level to 5-bit, effective shr 3

	; ax = I32_clut[(ax*65536)+dx]
	and eax,0x000000f8				; ax = (ax)
	shl eax,13						; ax = (ax * 65536)
	add eax,edx						; ax = (ax * 65536)+dx
	shl eax,1						; align to 16 bit array
	add eax,ebx						; I32_clut[(ax * 65536)+dx]
	mov ax,[eax]					; ax = I32_clut[(ax * 65536)+dx]
	stosw
;	mov [edi],ax					; Write output
;	add edi,2							; inc word output

	dec ecx
	jnz .darkloop

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

; Bitmap Combining

; 16bpp

	ALIGN 4

_darken_asm_blit16:
 darken_asm_blit16:
	push ebp
	mov ebp,esp

	push edi
	push esi

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	mov ecx,[ebp+16]		;Len

	ALIGN 4

.blitloop:
	lodsw
	test ax,ax
	jz .blitskip
	mov [edi],ax
	ALIGN 4
.blitskip:
	add edi,2
	loop .blitloop

	pop esi
	pop edi
	pop ebp
	ret

; 32bpp

	ALIGN 4

_darken_asm_blit32:
 darken_asm_blit32:
	push ebp
	mov ebp,esp

	push edi
	push esi

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	mov ecx,[ebp+16]		;Len

	ALIGN 4

.blitloop:
	lodsd
	test eax,eax
	jz .blitskip
	mov [edi],eax

	ALIGN 4
	
.blitskip:
	add edi,4
	dec ecx
	jnz .blitloop

	pop esi
	pop edi
	pop ebp
	ret

; Darken Sprite (always 32x32)

	ALIGN 4

_darken_asm_darksprite:
 darken_asm_darksprite:

	push ebp
	mov ebp,esp

	push edi
	push esi

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	;mov ???,[ebp+16]		;Len
	mov edx,[ebp+20]		;width offset

	mov ecx,32		; 32 lines

	ALIGN 4

darkspriteloop:
	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	paddusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	paddusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	paddusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	paddusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	add edi, edx
	
	dec cl
	jnz darkspriteloop

	pop esi
	pop edi

	pop ebp
	ret

; Lighten Sprite (always 32x32)

	ALIGN 4

_darken_asm_lightsprite:
 darken_asm_lightsprite:

	push ebp
	mov ebp,esp

	push edi
	push esi

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	;mov ???,[ebp+16]		;Len
	mov edx,[ebp+20]		;width offset

	mov ecx,32		; 32 lines

	ALIGN 4

lightspriteloop:
	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	psubusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	psubusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	psubusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	movq mm0,[edi]       ; get the dest pixels
	movq mm1,[esi]       ; get the source pixels
	psubusb mm0,mm1
	movq [edi],mm0		; Write output
	add edi,8
	add esi,8

	add edi, edx
	
;	loop lightspriteloop
	dec cl
	jnz lightspriteloop

	pop esi
	pop edi
	pop ebp
	ret
