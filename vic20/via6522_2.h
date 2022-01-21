/*  bad6502 A Raspberry Pi-based backend to a 65C02 CPU
    Copyright (C) 2022  D.Herrendoerfer

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>. 
*/

#include <stdlib.h>
#include <stdint.h>

#define bool _Bool
#define true 1
#define false 0

// VIA (6522 - Versatile Interface Adapter)

#ifndef VIA6522_2_H
#define VIA6522_2_H 1

#ifndef VIA_DEFINES

// VIA registers
#define VIA_REG_ORB_IRB         0x0
#define VIA_REG_ORA_IRA         0x1
#define VIA_REG_DDRB            0x2
#define VIA_REG_DDRA            0x3
#define VIA_REG_T1_C_LO         0x4
#define VIA_REG_T1_C_HI         0x5
#define VIA_REG_T1_L_LO         0x6
#define VIA_REG_T1_L_HI         0x7
#define VIA_REG_T2_C_LO         0x8
#define VIA_REG_T2_C_HI         0x9
#define VIA_REG_SR              0xa
#define VIA_REG_ACR             0xb   // Auxiliary Control Register
#define VIA_REG_PCR             0xc   // Peripherical Control Register
#define VIA_REG_IFR             0xd   // Interrupt Flag Register
#define VIA_REG_IER             0xe   // Interrupt Enable Register
#define VIA_REG_ORA_IRA_NH      0xf

// IER: VIA interrupt enable/disable bit mask
#define VIA_IER_CA2             0x01
#define VIA_IER_CA1             0x02
#define VIA_IER_SR              0x04
#define VIA_IER_CB2             0x08
#define VIA_IER_CB1             0x10
#define VIA_IER_T2              0x20
#define VIA_IER_T1              0x40
#define VIA_IER_CTRL            0x80  // 0 = Logic 1 in bits 0-6 disables the corresponding interrupt, 1 = Logic 1 in bits 0-6 enables the corresponding interrupt
 
// VIA, ACR flags
#define VIA_ACR_T2_COUNTPULSES  0x20
#define VIA_ACR_T1_FREERUN      0x40
#define VIA_ACR_T1_OUTENABLE    0x80

#define VIA_DEFINES 1
#endif
 
void via2_reset();
void via2_writeReg(int reg, int value);
int via2_readReg(int reg);
bool via2_tick(int cycles);
 
// set/get "external" state of PA
uint8_t via2_PA();
void via2_setPA(int value);
void via2_setBitPA(int bit, bool value);
void via2_clearBitPA(int bit);
 
// set/get "external" state of PB
uint8_t via2_PB();
void via2_setPB(int value);
void via2_setBitPB(int bit, bool value);
void via2_clearBitPB(int bit);

uint8_t via2_CA1();
void via2_setCA1(int value);
 
uint8_t via2_CA2();
void via2_setCA2(int value);

uint8_t via2_CB1();
void via2_setCB1(int value);
 
uint8_t via2_CB2();
void via2_setCB2(int value);

uint8_t via2_DDRA(); 
uint8_t via2_DDRB();
#endif
