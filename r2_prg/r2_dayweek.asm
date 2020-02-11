;R1 Content: 1-9 Numbers respectively / 10 Zero / 0 Still waiting
;MEM USAGE |0x0500-0x0503 YEAR| |0x0504-0x0505 MONTH| |0x0506-0x0507 DAY|
		send	r0,0x1002	;Cursor 0,0 (0x10YX)
		send	r0,0x200F	;Color W on B
		send	r0,'Y'
		send	r0,'Y'
		send	r0,'Y'
		send	r0,'Y'
		send	r0,'/'		;Sending date like this is ugly, but this is the fastest way. Also requires the least space.
		send	r0,'M'
		send	r0,'M'
		send	r0,'/'
		send	r0,'D'
		send	r0,'D'
		send	r0,0x1022
		send	r0,0x200D	;Color P on B	- You can spot the color selector easily
;YEAR INPUT BEGIN
		mov		r2,0x0500
YLoop:	cmp		r1,0		;R1 will get updated by overwriting with DRAY. R1 = Numpad last button press.
		je		YLoop
		mov		[r2],r1
		send	r0,r1		;Display on screen. The screen ROM has been altered so sending 1-9 will give '1'-'9' and 10 will give '0'
		mov		r1,0		;Reset R1 ASAP, so if the year is for example 2011 the user can press '1' twice quickly after eachother.
		add		r2,1
		cmp		r2,0x0504	;Year is 4 number.
		jl		YLoop
;YEAR INPUT END
		send	r0,0x200F
		send	r0,'/'		;W on B - print '/' - P on B
		send	r0,0x200D
;MONTH INPUT BEGIN
		mov		r2,0x0504
MLoop:	cmp		r1,0
		je		MLoop
		mov		[r2],r1
		send	r0,r1
		mov		r1,0
		add		r2,1
		cmp		r2,0x0506
		jl		MLoop
;MONTH INPUT END
		send	r0,0x200F
		send	r0,'/'
		send	r0,0x200D
;DAY INPUT BEGIN
		mov		r2,0x0506
DLoop:	cmp		r1,0
		je		DLoop
		cmp		r1,10
		jne		NonZ3
		sub		r1,10
NonZ3:	mov		[r2],r1
		add		r1,'0'
		send	r0,r1
		mov		r1,0
		add		r2,1
		cmp		r2,0x0508
		jl		DLoop
;DAY INPUT END
		send	r0,0x1040
		send	r0,0x200A
		send	r0,'C'
		send	r0,'a'
		send	r0,'l'
		send	r0,'c'
		send	r0,'u'
		send	r0,'l'
		send	r0,'a'
		send	r0,'t'
		send	r0,'i'
		send	r0,'n'
		send	r0,'g'
;YEAR D2B						QUICK DEC to BIN using tables
		mov		r5,[0x0500]
		mov		r2,[TABLE4+r5]
		mov		r5,[0x0501]
		add		r2,[TABLE3+r5]
		mov		r5,[0x0502]
		add		r2,[TABLE2+r5]
		mov		r5,[0x0503]
		add		r2,[TABLE1+r5]
;MONTH D2B
		mov		r5,[0x0504]
		mov		r3,[TABLE2+r5]
		mov		r5,[0x0505]
		add		r3,[TABLE1+r5]
		
		cmp		r3,1
		jl		MONerr		;Check if its in range
		cmp		r3,12
		ja		MONerr
;DAY D2B
		mov		r5,[0x0506]
		mov		r4,[TABLE2+r5]
		mov		r5,[0x0507]
		add		r4,[TABLE1+r5]
		
		cmp		r4,1
		jl		DAYerr		;Check if its in range
		cmp		r4,31
		ja		DAYerr
;READY					Actual calculaion of Day Of The Week. See:"https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Sakamoto's_methods"
		cmp		r3,3
		jnl		NOSUB
		sub		r2,1
NOSUB:	mov		r5,r2
		shr		r2,2	;divide 4
		add		r5,r2	;add y/4
		mov		r6,0
		
LADIV:	cmp		r2,250	;-|
		jl		DIVIS	; | Get larger numbers smaller quickier
		sub		r2,250	; |
		add		r6,10	; |
		jmp		LADIV	;-|
		
DIVIS:	cmp		r2,25
		jl		NoDivM
		add		r6,1
		sub		r2,25	;100/4==25
		jmp		DIVIS
NoDivM:
		
		sub		r5,r6				;sub y/100
		shr		r6,2				;div 100 TO div 400
		add		r5,r6				;add y/400
		add		r5,[TABLEC+r3]		;add table[m]
		add		r5,r4				;add d
		;Only mod 7 remaining.
NoLar:	cmp		r5,1211				;~Year 1950 / 2
		jl		NSubX
		sub		r5,1211
		jmp		NoLar
NSubX:	cmp		r5,70
		jl		Almost
		sub		r5,70
		jmp		NSubX
Almost:	cmp		r5,7
		jl		Done
		sub		r5,7
		jmp		Almost
;R5 is in range 0-6: 0 Sunday / 1 Monday / 2 TuesDay / ...
Done:	send	r0,0x200D
		send	r0,0x1063
		mov		r6,DayTable		;--|
		add		r6,[TABLE2+r5]	;  | Calculate address (multiples of 10)
		jmp		r6				;--|
		
TABLEC:	nop		;I couldn't find a way to subtract one from a label
		dw		0,3,2,5,0,3,5,1,4,6,2,4		;Table, see wikipedia link above
;REG USAGE: |R0-PORT_ZERO| |R1-NUMPAD| |R2 Y / R3 M / R4 D| |R5 SUM --> mod 7 --> Result|
DayTable:
Sunday:	send	r0,'S'
		send	r0,'u'
		send	r0,'n'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
		nop
		nop				;Padding to make selecting smaller, and faster
		nop
Mond:	send	r0,'M'
		send	r0,'o'
		send	r0,'n'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
		nop
		nop
		nop
Tuesd:	send	r0,'T'
		send	r0,'u'
		send	r0,'e'
		send	r0,'s'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
		nop
		nop
Wednesd:send	r0,'W'	;Longest - 10 cell ---> Perfect, can use TABLE2 instead of multiply/another table.
		send	r0,'e'
		send	r0,'d'
		send	r0,'n'
		send	r0,'e'
		send	r0,'s'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
Thursd:	send	r0,'T'
		send	r0,'h'
		send	r0,'u'
		send	r0,'r'
		send	r0,'s'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
		nop
Friday:	send	r0,'F'
		send	r0,'r'
		send	r0,'i'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
		nop
		nop
		nop
Saturd:	send	r0,'S'
		send	r0,'a'
		send	r0,'t'
		send	r0,'u'
		send	r0,'r'
		send	r0,'d'
		send	r0,'a'
		send	r0,'y'
		jmp		HALT
		nop
		
MONerr:	send	r0,0x200C
		send	r0,0x1060		;print errors in red color
		send	r0,'M'
		send	r0,'o'
		send	r0,'n'
		send	r0,'t'
		send	r0,'h'
		send	r0,' '
		send	r0,'n'
		send	r0,'o'
		send	r0,'t'
		send	r0,' '
		send	r0,'1'
		send	r0,'-'
		send	r0,'1'
		send	r0,'2'
		send	r0,'.'
		jmp		HALT
DAYerr:	send	r0,0x200C
		send	r0,0x1060
		send	r0,'D'
		send	r0,'a'
		send	r0,'y'
		send	r0,' '
		send	r0,'n'
		send	r0,'o'
		send	r0,'t'
		send	r0,' '
		send	r0,'1'
		send	r0,'-'
		send	r0,'3'
		send	r0,'1'
		send	r0,'.'
		jmp		HALT
HALT:	send	r0,0x1090
		send	r0,0x200A
		send	r0,'P'
		send	r0,'r'
		send	r0,'e'
		send	r0,'s'
		send	r0,'s'
		send	r0,' '
		send	r0,'"'
		send	r0,'R'
		send	r0,'"'
		send	r0,' '
		send	r0,'t'
		send	r0,'o'
		send	r0,0x10A0
		send	r0,'r'
		send	r0,'e'
		send	r0,'s'
		send	r0,'t'
		send	r0,'a'
		send	r0,'r'
		send	r0,'t'
		send	r0,'.'
		send	r0,0x10AA
		send	r0,'-'
		send	r0,'-'
		send	r0,'-'
		send	r0,'>'
		hlt
;DEC TO BIN conversion tables
TABLE4:	dw		0,1000,2000,3000,4000,5000,6000,7000,8000,9000,0
TABLE3: dw		0,100,200,300,400,500,600,700,800,900,0
TABLE2:	dw		0,10,20,30,40,50,60,70,80,90,0
TABLE1: dw		0,1,2,3,4,5,6,7,8,9,0
;MEM  |0x0500-0x0503 YEAR| |0x0504-0x0505 MONTH| |0x0506-0x0507 DAY|

;Register 1 FILT pixel location - 187X 178Y - NUM IN with DRAY