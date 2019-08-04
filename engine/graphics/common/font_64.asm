;
;	AMD64-ABI Assembler versions of the font routines
;
;	This will need modification to run in Windows
;	because they rolled their own ABI
;

[BITS 64]
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

SMARTALIGN


;   Monochrome bitpacked fonts

; 32bpp

ALIGN 16

_draw_bitfont_32asm:
draw_bitfont_32asm:

	; RDI = dest (framebuffer)
	; RSI = font data (8 bytes per character)
	; RDX = w offset (not for long!)
	; RCX = colour

	; Get width offset and convert to dwords, store in R8
	mov r8,rdx
	shl r8,2
	mov rdx,rcx	; Move the colour into EDX

	; R8 = bytes from end of row to start of next
	; R9 = h
	; CL = w
	; EDX = colour

	mov r9,8	; 8 pixels high

	ALIGN 16
.bitfonth:
	mov cl,8	; Set to 8 pixels wide
	lodsb		; Get row of 8x8 font into AL
	ALIGN 16
.bitfontw:
	shl al,1		; Shift AL left so the top bit goes into CARRY
	jnc .bitfontskip	; Nothing there?  Don't draw the pixel
	mov [rdi],edx		; Putpixel
	ALIGN 16
.bitfontskip:
	add rdi,4		; Move along one pixel
	dec cl
	jnz .bitfontw		; End of row?

	add rdi, r8		; Increment DI to start of next row
	dec r9
	jnz .bitfonth		; End of last line?
	ret

; 16bpp

ALIGN 16

_draw_bitfont_16asm:
draw_bitfont_16asm:
	push rbx

	; RDI = dest
	; RSI = font data
	; RDX = w offset (not for long!)
	; RCX = colour

	; Get width offset and convert to dwords
	mov r8,rdx
	shl r8,1
	mov rbx,rcx

	; RDX = h
	; RCX = w
	; RBX = colour

	xor rax,rax	; Clear high words
	mov rdx,8	; 8 pixels high

.bitfonth:

	mov rcx,8	; Set to 8 pixels wide
	
	lodsb
.bitfontw:
	test rax,128	; Test high bit of AL
	jz .bitfontskip
	mov [rdi],bx       ; Putpixel
.bitfontskip:
	add rdi,2
	shl rax,1	; Slide AL across to get next bit
	loop .bitfontw

	add rdi, r8
	dec rdx
	jnz .bitfonth

	pop rbx
	ret



;   Monochrome sprite fonts

; 32bpp

ALIGN 16

_draw_bytefont_32asm:
draw_bytefont_32asm:

	push rbx

	; RDI = dest
	; RSI = font data
	; RDX = w (not for long!)
	; RCX = h
	; R8 = offset
	; R9 = colour
	
	xchg rdx,rcx	; It would be nicer to have the width in RCX
	mov rbx,r9	; And we want the colour in RBX

	; RDX = h
	; RCX = w
	; RBX = colour

	mov r9,rcx	; Preserve width
	shl r8,2	; convert to dwords
	xor rax,rax	; Clear high words

bytefont32h:

	mov rcx,r9	; Restore width
	
bytefont32w:
	
	lodsb
	test rax,rax
	jz bytefont32skip
	mov [rdi],ebx       ; Putpixel
bytefont32skip:
	add rdi,4
	loop bytefont32w

	add rdi, r8
	dec rdx
	jnz bytefont32h

	pop rbx
	ret

; 16bpp

ALIGN 16

_draw_bytefont_16asm:
draw_bytefont_16asm:

	push rbx

	; RDI = dest
	; RSI = font data
	; RDX = w (not for long!)
	; RCX = h
	; R8 = offset
	; R9 = colour
	
	xchg rdx,rcx	; It would be nicer to have the width in RCX
	mov rbx,r9	; And we want the colour in RBX

	; RDX = h
	; RCX = w
	; RBX = colour

	mov r9,rcx	; Preserve width
	shl r8,1	; convert to words
	xor rax,rax	; Clear high words

bytefont16h:

	mov rcx,r9	; Restore width
	
bytefont16w:
	
	lodsb
	test rax,rax
	jz bytefont16skip
	mov [rdi],bx       ; Putpixel
bytefont16skip:
	add rdi,2
	loop bytefont16w

	add rdi, r8
	dec rdx
	jnz bytefont16h

	pop rbx
	ret


;   Colour sprite fonts

; 32bpp

ALIGN 16

_draw_colourfont_32asm:
draw_colourfont_32asm:

	push rbx

	; RDI = dest
	; RSI = font data
	; RDX = palette (dword pointer)
	; RCX =w
	; R8 = h
	; R9 = offset

	mov rbx,rdx	; Palette in RBX
	mov rdx,r8	; Height in RDX
	mov r8,r9
	mov r9,rcx	; Width in R9

	; RBX = pal
	; RDX = height
	; R8 = offset
	; R9 = width copy

	mov r9,rcx	; Preserve width
	shl r8,2	; convert to dwords
	xor rax,rax	; Clear high words

colourfont32h:

	mov rcx,r9	; Restore width
	
colourfont32w:
	
	lodsb
	test rax,rax
	jz colourfont32skip
	shl rax,2
	mov eax,[rbx+rax]
	mov [rdi],eax       ; Putpixel
	xor rax,rax
colourfont32skip:
	add rdi,4
	loop colourfont32w

	add rdi, r8
	dec rdx
	jnz colourfont32h

	pop rbx
	ret

; 16bpp

ALIGN 16

_draw_colourfont_16asm:
draw_colourfont_16asm:


	push rbx

	; RDI = dest
	; RSI = font data
	; RDX = palette (dword pointer)
	; RCX =w
	; R8 = h
	; R9 = offset

	mov rbx,rdx	; Palette in RBX
	mov rdx,r8	; Height in RDX
	mov r8,r9
	mov r9,rcx	; Width in R9

	; RBX = pal
	; RDX = height
	; R8 = offset
	; R9 = width copy

	mov r9,rcx	; Preserve width
	shl r8,1	; convert to words
	xor rax,rax	; Clear high words

colourfont16h:

	mov rcx,r9	; Restore width
	
colourfont16w:
	
	lodsb
	test rax,rax
	jz colourfont16skip
	shl rax,2
	mov eax,[rbx+rax]
	mov [rdi],ax       ; Putpixel
	xor rax,rax
colourfont16skip:
	add rdi,2
	loop colourfont16w

	add rdi, r8
	dec rdx
	jnz colourfont16h

	pop rbx
	ret
