;
;       Optional Assembly bitmask code.
;       This is only used if you #define USE_ASM in ire_game
;
;       Written more for amusement than for speed :-)
;

[BITS 32]

[global _SetBit_ASM]
[global _ResetBit_ASM]
[global _GetBit_ASM]

[EXTERN _cmap_width]
[EXTERN _cmap_height]

[SECTION .text]

;
;  Set a bit in the collision matrix
;

_SetBit_ASM:
    push ebp
    mov ebp,esp
    pushad

    mov eax, [_cmap_width]
    mov edi, [ebp+16]           ; EDI is the collision matrix
    mov edx, [ebp+12]           ; Get Y coordinate
    mov ebx, [ebp+8]            ; Get X coordinate

    mul edx                     ; Get offset, Y*width
    add edi, eax                ; EDI now points to the right byte column

    mov ecx, ebx                ; Save the X coordinate in CX
    shr ebx, 3                  ; Divide by 8, to get the byte
    add edi, ebx                ; Add the X coordinate

    and cl,  7                  ; Get the bit offset, 0-7 from the remainder
    mov al,  1                  ; Set AL to be one, and shift it by CL
    shl al,  cl                 ; Now we have the bitmask for ORing
    or  [edi], al               ; Set the bit we want

    popad
    pop ebp
    ret

;
;  Reset a bit in the collision matrix
;

_ResetBit_ASM:
    push ebp
    mov ebp,esp
    pushad

    mov eax, [_cmap_width]
    mov edi, [ebp+16]           ; EDI is the collision matrix
    mov edx, [ebp+12]           ; Get Y coordinate
    mov ebx, [ebp+8]            ; Get X coordinate

    mul edx                     ; Get offset, Y*width
    add edi, eax                ; EDI now points to the right byte column

    mov ecx, ebx                ; Save the X coordinate in CX
    shr ebx, 3                  ; Divide by 8, to get the byte
    add edi, ebx                ; Add the X coordinate

    and cl,  7                  ; Get the bit offset, 0-7 from the remainder
    mov al,  1                  ; Set AL to be one, and shift it by CL
    shl al,  cl                 ; Now we have the bitmask for ORing
    xor al,255                  ; Invert it
    and  [edi], al              ; Set the bit we want

    popad
    pop ebp
    ret

;
;  Get the state of an element in the collision matrix
;

_GetBit_ASM:
    push ebp
    mov ebp,esp

    xor eax,eax                 ; Zero EAX for the return

    pushad
    mov edi, [ebp+16]           ; EDI is the collision matrix
    mov eax, [_cmap_width]
    mov edx, [ebp+12]           ; Get Y coordinate
    mov ebx, [ebp+8]            ; Get X coordinate

    mul edx                     ; Get offset, Y*width
    add edi, eax                ; EDI now points to the right byte column

    mov ecx, ebx                ; Save the X coordinate in CX
    shr ebx, 3                  ; Divide by 8, to get the byte
    add edi, ebx                ; Add the X coordinate

    and cx,  7                  ; Get the bit offset, 0-7 from the remainder

    mov al,  [edi]              ; Copy the interesting byte into al
    btc ax,cx			; Get the contents of bit CL in byte AL
                                ; Carry Flag is now set to this value
    popad                       ; EAX returns to zero
    pop ebp
    salc			; SET AL=CARRY - http://www.x86.org
    ret                         ; SALC is undocumented but present in all
                                ; known x86 compatible microprocessors




