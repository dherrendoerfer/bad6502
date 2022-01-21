;Hello World for bad6502
BC = $10
DB = $11
S1 = $1000
	JSR PRIMM
	.BYTE "Test program running !",10,00
	JSR PRIMM
	.BYTE "...zero mem",10,00
        LDA #$00
	LDX #$00
LOOP0
        STA $A000,X
	INX
	BNE LOOP0

	JSR PRIMM
	.BYTE "...write mem",10,00

        LDA #$00
	LDX #$00
LOOP1
        STA $A000,X 
	ADC #$01
	INX
	BNE LOOP1 

	JSR PRIMM
	.BYTE "...read mem",10,00

	LDX #$00
LOOP2
	STX $15
	LDA $A000,X
	CMP $15
	BNE ERRMSG
	INX
	BNE END1
	JMP LOOP2
ERRMSG
	JSR PRIMM
	.BYTE "...read err",10,00
	JMP STOP

END1
	JSR PRIMM
	.BYTE "...done",10,00

; Wait loop
	LDX #$FF      ;init loop counter
	LDY #$FF      ;init loop counter
L3
    	DEX
    	BNE L3
    	DEY
    	BNE L3
    	
	JMP S1

STOP
	JMP STOP

;Library of functions

PRIMM:
	PHA     		; save A
	TYA			; copy Y
	PHA  			; save Y
	TXA			; copy X
	PHA  			; save X
	TSX			; get stack pointer
	LDA $0104,X		; get return address low byte (+4 to
				;   correct pointer)
	STA $BC			; save in page zero
	LDA $0105,X		; get return address high byte (+5 to
				;   correct pointer)
	STA $BD			; save in page zero
	LDY #$01		; set index (+1 to allow for return
				;   address offset)
PRIM2:
	LDA ($BC),Y		; get byte from string
	BEQ PRIM3		; exit if null (end of text)

	STA $0E000		; else display character
	INY			; increment index
	BNE PRIM2		; loop (exit if 256th character)

PRIM3:
	TYA			; copy index
	CLC			; clear carry
	ADC $BC			; add string pointer low byte to index
	STA $0104,X		; put on stack as return address low byte
				; (+4 to correct pointer, X is unchanged)
	LDA #$00		; clear A
	ADC $BD		; add string pointer high byte
	STA $0105,X		; put on stack as return address high byte
				; (+5 to correct pointer, X is unchanged)
	PLA			; pull value
	TAX  			; restore X
	PLA			; pull value
	TAY  			; restore Y
	PLA  			; restore A
	RTS

