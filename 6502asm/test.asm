;Hello World for bad6502
S1 = $1000
    LDY #$00      ;init loop counter
L0
    LDA L1,Y      ;load a byte of the text
    BEQ L2        ;if zero -> end
    STA $100      ;write to term (0x100)
    INY           ;inc loop counter
    JMP L0 
L1
    .byte "hello world: the quick brown fox jumps over the lazy dog",10,0
L2
    JMP L2
