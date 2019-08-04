;
;	i386 Assembler versions of the font routines
;

[BITS 32]
[SECTION .text]

; Declare public symbols

GLOBAL _draw_bitfont_32asm
GLOBAL draw_bitfont_32asm
GLOBAL _draw_bitfont_16asm
GLOBAL draw_bitfont_16asm
GLOBAL _draw_bytefont_32asm
GLOBAL draw_bytefont_32asm
GLOBAL _draw_bytefont_16asm
GLOBAL draw_bytefont_16asm
GLOBAL _draw_colourfont_32asm
GLOBAL draw_colourfont_32asm
GLOBAL _draw_colourfont_16asm
GLOBAL draw_colourfont_16asm

;   Monochrome bitpacked fonts

; 32bpp

_draw_bitfont_32asm:
draw_bitfont_32asm:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	mov ebx,[ebp+20]		;colour

	; Get width offset and convert to dwords
	mov ebp,[ebp+16];
	shl ebp,2
	mov dl,8		; 8 pixels high

.bitfonth:

	mov cl,8	; Set to 8 pixels wide
	
	lodsb
	ALIGN 4
.bitfontw:
	shl al,1			; Move top bit into carry
	jnc .bitfontskip	; 
	mov [edi],ebx       ; Putpixel
	ALIGN 4
.bitfontskip:
	add edi,4
	dec cl
	jnz .bitfontw

	add edi,ebp
	dec dl
	jnz .bitfonth

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

; 16 bpp

	ALIGN 4
	
_draw_bitfont_16asm:
draw_bitfont_16asm:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx

	mov edi,[ebp+8]			;Dest
	mov esi,[ebp+12]		;Source
	mov ebx,[ebp+20]		;colour

	; Get width offset and convert to words
	mov ebp,[ebp+16];
	shl ebp,1
	mov dl,8		; 8 pixels high

.bitfonth:

	mov cl,8	; Set to 8 pixels wide
	
	lodsb
.bitfontw:
	shl al,1			; Move top bit into carry
	jnc .bitfontskip	; 
	mov [edi],bx       ; Putpixel
.bitfontskip:
	add edi,2
	dec cl
	jnz .bitfontw

	add edi,ebp
	dec dl
	jnz .bitfonth

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

;   Monochrome sprite fonts

; 32bpp

	ALIGN 4

_draw_bytefont_32asm:
draw_bytefont_32asm:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx

	mov esi,[ebp+8]		;Dest	SCASB wants to use DI :-(
	mov edi,[ebp+12]		;font data
;	mov ecx,[ebp+16]		;width
	mov edx,[ebp+20]		;height
	mov ecx,[ebp+24]		;offset
	mov ebx,[ebp+28]		;colour

	; Get width offset and convert to dwords
	shl ecx,2
	mov [widthoffset],ecx
	
	xor eax,eax	; Clear high words

.bytefonth:

	mov ecx,[ebp+16]		;width
	
.bytefontw:
	scasb				; make sure AL is clear!
	jz .bytefontskip
	mov [esi],ebx       ; Putpixel
.bytefontskip:
	add esi,4
	dec cl
	jnz .bytefontw

	add esi, [widthoffset]
	dec dl
	jnz .bytefonth

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

; 16 bpp

	ALIGN 4

_draw_bytefont_16asm:
draw_bytefont_16asm:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx

	mov esi,[ebp+8]		;Dest	SCASB wants to use DI :-(
	mov edi,[ebp+12]		;font data
;	mov ecx,[ebp+16]		;width
	mov edx,[ebp+20]		;height
	mov ecx,[ebp+24]		;offset
	mov ebx,[ebp+28]		;colour

	; Get width offset and convert to words
	shl ecx,1
	mov [widthoffset],ecx
	
	xor eax,eax	; Clear high words

.bytefonth:

	mov ecx,[ebp+16]		;width
	
.bytefontw:
	scasb				; make sure AL is clear!
	jz .bytefontskip
	mov [esi],bx       ; Putpixel
.bytefontskip:
	add esi,2
	dec cl
	jnz .bytefontw

	add esi, [widthoffset]
	dec dl
	jnz .bytefonth

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret


;   Colour sprite fonts

; 32bpp

	ALIGN 4

_draw_colourfont_32asm:
draw_colourfont_32asm:

	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx
	mov edi,[ebp+8]		;Dest
	mov esi,[ebp+12]		;font data
	mov ebx,[ebp+16]		;palette
;	mov eax,[ebp+20]		;width
	mov edx,[ebp+24]		;height
	mov ecx,[ebp+28]		;offset

	; Get width offset and convert to dwords
	shl ecx,2
	mov [widthoffset],ecx
	mov ecx,[ebp+20]	; Restore width
	mov ch,cl

	xor eax,eax	; Clear high words

.colourfonth:
	mov cl,ch	
.colourfontw:
	
	lodsb
	test al,al
	jz .colourfontskip
	mov eax,[ebx+eax*4]	; lookup table
	mov [edi],eax       		; Putpixel
	xor eax,eax
.colourfontskip:
	add edi,4
	dec cl
	jnz .colourfontw

	add edi, [widthoffset]
	dec dl
	jnz .colourfonth

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret

	ALIGN 4

_draw_colourfont_16asm:
draw_colourfont_16asm:
	push ebp
	mov ebp,esp

	push edi
	push esi
	push ebx
	mov edi,[ebp+8]		;Dest
	mov esi,[ebp+12]		;font data
	mov ebx,[ebp+16]		;palette
;	mov eax,[ebp+20]		;width
	mov edx,[ebp+24]		;height
	mov ecx,[ebp+28]		;offset

	; Get width offset and convert to words
	shl ecx,1
	mov [widthoffset],ecx
	mov ecx,[ebp+20]	; Restore width
	mov ch,cl

	xor eax,eax	; Clear high words

.colourfonth:
	mov cl,ch	
.colourfontw:
	
	lodsb
	test al,al
	jz .colourfontskip
	mov eax,[ebx+eax*4]	; lookup table
	mov [edi],ax       		; Putpixel
	xor eax,eax
.colourfontskip:
	add edi,2
	dec cl
	jnz .colourfontw

	add edi, [widthoffset]
	dec dl
	jnz .colourfonth

	pop ebx
	pop esi
	pop edi
	pop ebp
	ret


[SECTION .data]
	; Register spill
widthoffset:  times 1 dd 0

