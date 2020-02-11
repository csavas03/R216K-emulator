;1 TAB = 4 space
;R1 Content: 1-9 Numbers respectively / 10 Zero / 11 Enter / 0 Waiting for button press
;       No idea why I haven't made it 0.--^
		mov		r0,0
		mov		r1,0
		send	r0,0x1000	;Cursor 0,0 (0x10YX)
		send	r0,0x200F	;Color
		send	r0,'A'
		send	r0,'='
		send	r0,0x200D	;Color
		
		mov		r2,NUMAB
INA:	cmp		r1,0
		je		INA
		cmp		r1,11
		jne		STRA
		cmp		r2,NUMAB
		jne		ADONE
		mov		r1,0
		jmp		INA
STRA:	mov		[r2],r1
		send	r0,r1
		add		r2,1
		mov		r1,0
		cmp		r2,NUMAL
		jne		INA
		
ADONE:	send	r0,0x1010
		send	r0,0x200F
		send	r0,'B'
		send	r0,'='
		send	r0,0x200D
		
		mov		r3,NUMBB
INB:	cmp		r1,0
		je		INB
		cmp		r1,11
		jne		STRB
		cmp		r3,NUMBB
		jne		BDONE
		mov		r1,0
		jmp		INB
STRB:	mov		[r3],r1
		send	r0,r1
		add		r3,1
		mov		r1,0
		cmp		r3,NUMBL
		jne		INB
		
BDONE:	send	r0,0x1020
		send	r0,0x200C
		send	r0,'C'
		send	r0,'a'
		send	r0,'l'
		send	r0,'c'
		send	r0,' '
		
		mov		r4,NUMAE
LOOPA:	sub		r4,1
		sub		r2,1
		mov		r5,[r2]
		cmp		r5,10
		jne		NSUB
		mov		r5,0
NSUB:	mov		[r4],r5
		cmp		r2,NUMAB
		jne		LOOPA
LOOPB:	cmp		r4,NUMAB
		jbe		ENDC
		sub		r4,1
		mov		[r4],0xFFFF
		jmp		LOOPB
		
ENDC:	send	r0,'A'
		
		mov		r4,NUMBE
LOOPE:	sub		r4,1
		sub		r3,1
		mov		r5,[r3]
		cmp		r5,10
		jne		NSUB2
		mov		r5,0
NSUB2:	mov		[r4],r5
		cmp		r3,NUMBB
		jne		LOOPE
LOOPF:	cmp		r4,NUMBB
		jbe		ENDG
		sub		r4,1
		mov		[r4],0xFFFF
		jmp		LOOPF
		
ENDG:	send	r0,'B'
		mov		r2,NUMCB
		
CLRR:	mov		[r2],0
		add		r2,1
		cmp		r2,NUMCE
		jne		CLRR
		
		send	r0,'C'
		
		mov		r2,NUMAL
		mov		r3,NUMBL
		mov		r4,NUMCL
		mov		r9,NUMCL
		;actual multiplication
MUL1:	mov		r6,[r3]			;BL/BLAdr
		send	r0,0x1028
		send	r0,r6
MUL2:	mov		r5,[r2]			;AL/ALAdr
		mov		r7,r6			;BL/BL
		add		r7,[MUL10T+r5]	;AL*10 + BL
		mov		r8,[MULTAB+r7]	;AL*BL
		;mov	r10,r4	/-------\
		;and	r10,7	|"DEBUG"|
		;send	r0,r10	|       |
		;send	r0,r8	\-------/
		add		[r4],r8
		sub		r2,1
		sub		r4,1
		cmp		[r2],0xFFFF
		jne		MUL2
		sub		r9,1
		mov		r4,r9
		mov		r2,NUMAL
		sub		r3,1
		cmp		[r3],0xFFFF
		jne		MUL1

		send	r0,'M'
		;Check for digits above 9 and carry
		mov		r2,NUMCE
		mov		r3,0
CARRY:	sub		r2,1
		cmp		r2,NUMCB
		jl		CARRE
		mov		r4,[r2]
		add		r4,r3
		mov		r3,0
		cmp		r4,10
		jnl		CARRL
		mov		[r2],r4
		jmp		CARRY
		
CARRL:	cmp		r4,250	;/------------------\
		jl		CARR5
		add		r3,25
		sub		r4,250
		jmp		CARRL	;	Can be removed
CARR5:	cmp		r4,50
		jl		CARR1
		add		r3,5
		sub		r4,50
		jmp		CARR5	;\------------------/
CARR1:	cmp		r4,10
		jl		GOOD
		add		r3,1
		sub		r4,10
		jmp		CARR1
GOOD:	mov		[r2],r4
		jmp		CARRY
		
CARRE:	
		send	r0,0x200F
		send	r0,0x1040
		send	r0,'A'
		send	r0,'*'
		send	r0,'B'
		send	r0,'='
		send	r0,0x200D
		
		mov		r2,NUMCB
LOOP:	cmp		[r2],0
		jne		LBL
		add		r2,1
		cmp		r2,NUMCL
		jl		LOOP
LBL:	
		sub		r2,NUMCB
		add		r2,RESPRNT
		mov		r3,NUMCB
		jmp		r2			;Loops are slow, just jump to the first digit to be printed

RESPRNT:send	r0,[r3+00]
		send	r0,[r3+01]
		send	r0,[r3+02]
		send	r0,[r3+03]
		send	r0,[r3+04]
		send	r0,[r3+05]
		send	r0,[r3+06]
		send	r0,[r3+07]
		send	r0,[r3+08]
		send	r0,[r3+09]
		send	r0,[r3+10]
		send	r0,[r3+11]
		send	r0,[r3+12]
		send	r0,[r3+13]
		send	r0,[r3+14]
		send	r0,[r3+15]
		send	r0,[r3+16]
		send	r0,[r3+17]
		send	r0,[r3+18]
		send	r0,[r3+19]
		send	r0,[r3+20]
		send	r0,[r3+21]
		send	r0,[r3+22]
		send	r0,[r3+23]
		send	r0,[r3+24]
		send	r0,[r3+25]
		send	r0,[r3+26]
		hlt

NUMAB:	dw		0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0	;A
NUMAL:	dw		0								;A
NUMAE:
NUMBB:	dw		0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0	;B
NUMBL:	dw		0								;B
NUMBE:	
NUMCB:	dw		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	;Result
NUMCL:	dw		0													;Result
NUMCE:	
;The next data array conatins a 9x9 multiplication table.
MULTAB:	dw		0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,0,2,4,6,8,10,12,14,16,18,0,3,6,9,12,15,18,21,24,27,0,4,8,12,16,20,24,28,32,36,0,5,10,15,20,25,30,35,40,45,0,6,12,18,24,30,36,42,48,54,0,7,14,21,28,35,42,49,56,63,0,8,16,24,32,40,48,56,64,72,0,9,18,27,36,45,54,63,72,81
MUL10T:	dw		0,10,20,30,40,50,60,70,80,90	;Calculate address for the array above quickly.
;Register 1 FILT pixel location - 187X 178Y - NUM IN with DRAY