; -------------------------------
;  hexdump.asm - by Cool4Cool	 
;  Hexdumps the contents of RAM
; -------------------------------
;  Various functions by Sopa-
;  XorzTaker, LBPHacker, and me
; -------------------------------
;  Requires the R216K2A, RTERM2, 
;  and the R2ASM assembler  
; -------------------------------


;--------------------------------------Main Program--------------------------------------

;Initialize program
initHexdump:
	
	;Setup terminal
	mov r11, 0		;Line counter
	mov r10, 0		;Serial port to terminal
	send r10, 0x1000	;Sets cursor to position 1,1
	send r10, 0x200A	;Change text color
	mov r2, 3		;Limit number of digits in input
	
	;Stack pointer
	mov sp, 0x800
	
	;Print title
	call print_newline
	mov r0, strings.title
	call print_string
	call print_newline
	call print_newline

	;Memory address counters
	mov r0, strings.offset1		;Print "Offset 1: "
	call print_string
	call read_hex			;Offset 1, register r1 (The counter) previously bx
	mov r1, r0
	call print_newline

	mov r0, strings.offset2		;Print "Offset 2: "
	call print_string
	call read_hex			
	mov r2, r0			;Offset 2, register r2 (The end) previously ax
	call print_newline
	call print_newline

	mov r3, 0			;Position counter, register r3 previously cx


;Main loop to print out contents of computer's RAM
;Format: [Address of first byte of row]: <byte> <byte> <byte> <byte>
readMemoryLoop:
	cmp r1, r2	;Compare the offset counter's value to set value (configure value: end of block), jump to halt if reaches maxmimum
	jge haltLoop

	cmp r1, 0x800	;Configured for 2048-byte R2, max address 0x800
	jge haltLoop
	
	mov r0, r1	;Print contents of offset counter (current address)
	call print_hex
	
	send r10, 58	;Print colon and space in ASCII
	send r10, 0x20	
	
	mov r3, 0	;Set position counter to zero 


;Sub-loop that prints all 4 bytes at offset	
positionLoop:	
	mov r0, [r1+r3]	;Print memory content at current address - [offset+position] and an ASCII space
	call print_hex
	send r10, 0x20

	add r3, 1	;Increment position counter to next byte in memory and jump to sub-loop if 4 bytes weren't printed yet
	cmp r3, 2
	jl positionLoop

	call print_newline	;Jump to a new line
	add r1, 2 		;Increment offset counter
	jmp readMemoryLoop
	

;Halt program
haltLoop:
	call print_newline
	mov r0, strings.halt
	call print_string
	hlt	


;--------------------------------------Subroutines--------------------------------------


; Reads a hexadecimal number into a register
; Args: r2 = max amount of digits
; Returns number in r0

read_hex:
	push r1		; Holds current character
	push r2		; Max amount of digits
	push r3		; Digit counter
	mov r0, 0	; Input hex
	mov r3, 0

	.loop:
	call read_character

	cmp r1, 0x0D	; Is the character enter?
	je .end
		
	cmp r1, 48	; 0 - 9?
	jl .loop	
	cmp r1, 57
	jle .hex_number

	cmp r1, 65	; A - F?
	jl .loop
	cmp r1, 70
	jle .hex_upper

	cmp r1, 97	; a - f?
	jl .loop
	cmp r1, 102
	jle .hex_lower
	jmp .loop

	.hex_number:	; Convert 0 - 9
	send r10, r1
	sub r1, 48
	jmp .write_hex

	.hex_upper:	; Convert A - F
	send r10, r1
	sub r1, 55
	jmp .write_hex
		
	.hex_lower:	; Convert a - f
	send r10, r1
	sub r1, 87
	jmp .write_hex
	
	.write_hex:	; Write digit
	add r3, 1
	shl r0, 4
	add r0, r1

	cmp r3, r2	; Is the character max reached?
	jge .end

	jmp .loop

	.end:
	pop r3
	pop r2
	pop r1
	ret


; TPT-IO function by Sopr0orzTaker
; Prints a hexadecimal number
; Args: r0 = number

print_hex:
	push r0
	push r1
	push r2
	push r3
	push r4

	mov r1, r0
	mov r2, r0
	mov r3, r0

	and r0, 0xF
	shr r1, 4
	and r1, 0xF
	shr r2, 8
	and r2, 0xF
	shr r3, 12
	and r3, 0xF

	mov r4, [r3+print_hex.hexdec]
	send r10, r4
	mov r4, [r2+print_hex.hexdec]
	send r10, r4
	mov r4, [r1+print_hex.hexdec]
	send r10, r4
	mov r4, [r0+print_hex.hexdec]
	send r10, r4

	pop r4
	pop r3
	pop r2
	pop r1
	pop r0
	ret
	.hexdec: dw "0123456789ABCDEF"


; Prints a string
; Args: r0 -> string[0]

print_string:
	push r0
	push r1
	
	.loop:
	mov r1, [r0]
	jz .end
	add r0, 1
	send r10, r1
	jmp .loop

	.end:
	pop r1
	pop r0
	ret


; Inputs a character
; Returns character in r1

read_character:
	.wait_loop:
	wait r10		
	js .wait_loop
	bump r10

	.recv_loop:
	recv r1, r10
	jnc .recv_loop
	ret


; Advances cursor to the next line
; Uses r11 register as a line counter

print_newline:
	add r11, 0x0001		; Adds 1 line to the counter (configure)	
	cmp r11, 0x000C		; If there are more than 12 lines, jump to
	jge .scrollUp		; the scroll up function (configure)
	.printNlEntryPoint:
	push r11		; Save the data held in r11
	shl r11, 0x0004		; Generate/send exact cursor location by shifting
	or r11, 0x1000		; the Y (stored in r11) to the left by 5 bits and
	send r10, r11		; performing a bitwise OR on it and the X 
	pop r11			; (configure), before ORing it with 0x1000
	ret			; Restore value of r11
	
	.scrollUp:
	push r5
	mov r5, 192		; Clear screen
	call clear_continuous
	mov r11, 0x0000		; Change value of r11 to 0 lines (configure)
	pop r5
	jmp .printNlEntryPoint	; Jump the entry point in the main part of function


; Clears the screen
; Args: r5 = number of spaces
clear_continuous:
	send r10, 0x1000
	.loop:
	send r10, 32
	sub r5, 1
	jnz .loop
	ret


;Stores all strings required by program
strings:
	.title: dw " C4C Hexdumper ", 0
	.offset1: dw "Offset 1: ", 0
	.offset2: dw "Offset 2: ", 0
	.halt: dw "Halt.", 0


;For demo purposes
org 0x200
dw 0xDEAD, 0xBEEF, 0xDEAD, 0xBEEF, 0xDEAD, 0xBEEF
