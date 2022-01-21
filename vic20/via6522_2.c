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

#include <stdio.h>
#include "via6522_2.h"
 
 
// VIA (6522 - Versatile Interface Adapter)

// timers
volatile int           via2__timer1Counter;
volatile uint16_t      via2__timer1Latch;
volatile int           via2__timer2Counter;
volatile uint8_t       via2__timer2Latch;     // timer 2 latch is 8 bits
volatile bool          via2__timer1Triggered;
volatile bool          via2__timer2Triggered;
 
// CA1, CA2
volatile uint8_t       via2__CA1;
volatile uint8_t       via2__CA1_prev;
volatile uint8_t       via2__CA2;
volatile uint8_t       via2__CA2_prev;
 
// CB1, CB2
volatile uint8_t       via2__CB1;
volatile uint8_t       via2__CB1_prev;
volatile uint8_t       via2__CB2;
volatile uint8_t       via2__CB2_prev;
 
// PA, PB
volatile uint8_t       via2__DDRA;
volatile uint8_t       via2__DDRB;
volatile uint8_t       via2__PA;     // what actually there is out of 6522 port A
volatile uint8_t       via2__PB;     // what actually there is out of 6522 port B
volatile uint8_t       via2__IRA;    // input register A
volatile uint8_t       via2__IRB;    // input register B
volatile uint8_t       via2__ORA;    // output register A
volatile uint8_t       via2__ORB;    // output register B
 
volatile uint8_t       via2__IFR;
volatile uint8_t       via2__IER;
volatile uint8_t       via2__ACR;
volatile uint8_t       via2__PCR;
volatile uint8_t       via2__SR;
 
#if DEBUG6522
volatile int           via2__tick;
#endif

#if DEBUG6522
static volatile const char * VIAREG2STR[] = { "ORB_IRB", "ORA_IRA", "DDRB", "DDRA", "T1_C_LO", "T1_C_HI", "T1_L_LO", "T1_L_HI", "T2_C_LO", "T2_C_HI", "SR", "ACR", "PCR", "IFR", "IER", "ORA_IRA_NH" };
#endif
 
void via2_reset()
{
   via2__timer1Counter   = 0x0000;
   via2__timer1Latch     = 0x0000;
   via2__timer2Counter   = 0x0000;
   via2__timer2Latch     = 0x00;
   via2__CA1             = 0;
   via2__CA1_prev        = 0;
   via2__CA2             = 0;
   via2__CA2_prev        = 0;
   via2__CB1             = 0;
   via2__CB1_prev        = 0;
   via2__CB2             = 0;
   via2__CB2_prev        = 0;
   via2__IFR             = 0;
   via2__IER             = 0;
   via2__ACR             = 0;
   via2__timer1Triggered = false;
   via2__timer2Triggered = false;
   via2__DDRA            = 0;
   via2__DDRB            = 0;
   via2__PCR             = 0;
   via2__PA              = 0xff;
   via2__PB              = 0xff;
   via2__SR              = 0;
   via2__IRA             = 0xff;
   via2__IRB             = 0xff;
   via2__ORA             = 0;
   via2__ORB             = 0;
}
 
 
void via2_setPA(int value)
{
   via2__PA = value;
   via2__IRA = via2__PA; // IRA is exactly what there is on port A
}
 
 
void via2_setBitPA(int bit, bool value)
{
   uint8_t newPA = (via2__PA & ~(1 << bit)) | ((int)value << bit);
   via2_setPA(newPA);
}
 
 
void via2_clearBitPA(int bit)
{
   uint8_t mask = (1 << bit);
   if (via2__DDRA & mask) {
     // pin configured as output, set value of ORA
     via2_setBitPA(bit, via2__ORA & mask);
   } else {
     // pin configured as input, pull it up
     via2_setBitPA(bit, true);
   }
}
 
 
void via2_setPB(int value)
{
   via2__PB = value;
   via2__IRB = (via2__PB & ~via2__DDRB) | (via2__ORB & via2__DDRB);  // IRB contains PB for inputs, and ORB for outputs
}
 
 
void via2_setBitPB(int bit, bool value)
{
   uint8_t newPB = (via2__PB & ~(1 << bit)) | ((int)value << bit);
   via2_setPB(newPB);
}
 
 
void via2_clearBitPB(int bit)
{
   uint8_t mask = (1 << bit);
   if (via2__DDRB & mask) {
     // pin configured as output, set value of ORB
     via2_setBitPB(bit, via2__ORB & mask);
   } else {
     // pin configured as input, pull it up
     via2_setBitPB(bit, true);
   }
}
 
 
// reg must be 0..0x0f (not checked)
void via2_writeReg(int reg, int value)
{
   #if DEBUG6522
   printf("\ntick %d VIA 2 writeReg(%s, 0x%02x)\n", via2__tick, VIAREG2STR[reg], value);
   #endif
 
   switch (reg) {
 
     // ORB: Output Register B
     case VIA_REG_ORB_IRB:
       via2__ORB = value;
       via2__PB  = (via2__ORB & via2__DDRB) | (via2__PB & ~via2__DDRB);
       via2__IRB = (via2__PB & ~via2__DDRB) | (via2__ORB & via2__DDRB);  // IRB contains PB for inputs, and ORB for outputs
       // clear CB1 and CB2 interrupt flags
       via2__IFR &= ~VIA_IER_CB1;
       via2__IFR &= ~VIA_IER_CB2;
       break;
 
     // ORA: Output Register A
     case VIA_REG_ORA_IRA:
       // clear CA1 and CA2 interrupt flags
       via2__IFR &= ~VIA_IER_CA1;
       via2__IFR &= ~VIA_IER_CA2;
       // not break!
     // ORA: Output Register A - no Handshake
     case VIA_REG_ORA_IRA_NH:
       via2__ORA = value;
       via2__PA  = (via2__ORA & via2__DDRA) | (via2__PA & ~via2__DDRA);
       via2__IRA = via2__PA;                                   // IRA is exactly what there is on port A
       break;
 
     // DDRB: Data Direction Register B
     case VIA_REG_DDRB:
       via2__DDRB = value;
       via2__PB   = (via2__ORB & via2__DDRB) | (via2__PB & ~via2__DDRB);   // refresh Port B status
       via2__IRB  = (via2__PB & ~via2__DDRB) | (via2__ORB & via2__DDRB);   // IRB contains PB for inputs, and ORB for outputs
       break;
 
     // DDRA: Data Direction Register A
     case VIA_REG_DDRA:
       via2__DDRA = value;
       via2__PA   = (via2__ORA & via2__DDRA) | (via2__PA & ~via2__DDRA);   // refresh Port A status
       via2__IRA  = via2__PA;                                  // IRA is exactly what there is on port A
       break;
 
     // T1C-L: T1 Low-Order Latches
     case VIA_REG_T1_C_LO:
       via2__timer1Latch = (via2__timer1Latch & 0xff00) | value;
       break;
 
     // T1C-H: T1 High-Order Counter
     case VIA_REG_T1_C_HI:
       via2__timer1Latch = (via2__timer1Latch & 0x00ff) | (value << 8);
       // timer1: write into high order counter
       // timer1: transfer low order latch into low order counter
       via2__timer1Counter = (via2__timer1Latch & 0x00ff) | (value << 8);
       // clear T1 interrupt flag
       via2__IFR &= ~VIA_IER_T1;
       via2__timer1Triggered = false;
       break;
 
     // T1L-L: T1 Low-Order Latches
     case VIA_REG_T1_L_LO:
       via2__timer1Latch = (via2__timer1Latch & 0xff00) | value;
       break;
 
     // T1L-H: T1 High-Order Latches
     case VIA_REG_T1_L_HI:
       via2__timer1Latch = (via2__timer1Latch & 0x00ff) | (value << 8);
       // clear T1 interrupt flag
       via2__IFR &= ~VIA_IER_T1;
       break;
 
     // T2C-L: T2 Low-Order Latches
     case VIA_REG_T2_C_LO:
       via2__timer2Latch = value;
       break;
 
     // T2C-H: T2 High-Order Counter
     case VIA_REG_T2_C_HI:
       // timer2: copy low order latch into low order counter
       via2__timer2Counter = (value << 8) | via2__timer2Latch;
       // clear T2 interrupt flag
       via2__IFR &= ~VIA_IER_T2;
       via2__timer2Triggered = false;
       break;
 
     // SR: Shift Register
     case VIA_REG_SR:
       via2__SR = value;
       break;
 
     // ACR: Auxliary Control Register
     case VIA_REG_ACR:
       via2__ACR = value;
       break;
 
     // PCR: Peripheral Control Register
     case VIA_REG_PCR:
     {
       via2__PCR = value;
       // CA2 control
       switch ((via2__PCR >> 1) & 0b111) {
         case 0b110:
           // manual output - low
           via2__CA2 = 0;
           break;
         case 0b111:
           // manual output - high
           via2__CA2 = 1;
           break;
         default:
           break;
       }
       // CB2 control
       switch ((via2__PCR >> 5) & 0b111) {
         case 0b110:
           // manual output - low
           via2__CB2 = 0;
           break;
         case 0b111:
           // manual output - high
           via2__CB2 = 1;
           break;
         default:
           break;
       }
       break;
     }
 
     // IFR: Interrupt Flag Register
     case VIA_REG_IFR:
       // reset each bit at 1
       via2__IFR &= ~value & 0x7f;
       break;
 
     // IER: Interrupt Enable Register
     case VIA_REG_IER:
       if (value & VIA_IER_CTRL) {
         // set 0..6 bits
         via2__IER |= value & 0x7f;
       } else {
         // reset 0..6 bits
         via2__IER &= ~value & 0x7f;
       }
       break;
   };
}
 
 
 // reg must be 0..0x0f (not checked)
int via2_readReg(int reg)
{
   #if DEBUG6522
   printf("\ntick %d VIA 2 readReg(%s)\n", via2__tick, VIAREG2STR[reg]);
   #endif
 
   switch (reg) {
 
     // IRB: Input Register B
     case VIA_REG_ORB_IRB:
       // clear CB1 and CB2 interrupt flags
       via2__IFR &= ~VIA_IER_CB1;
       via2__IFR &= ~VIA_IER_CB2;
       // get updated PB status
       return via2__IRB;
 
     // IRA: Input Register A
     case VIA_REG_ORA_IRA:
       // clear CA1 and CA2 interrupt flags
       via2__IFR &= ~VIA_IER_CA1;
       via2__IFR &= ~VIA_IER_CA2;
     // IRA: Input Register A - no handshake
     case VIA_REG_ORA_IRA_NH:
       // get updated PA status
       return via2__IRA;
 
     // DDRB: Data Direction Register B
     case VIA_REG_DDRB:
       return via2__DDRB;
 
     // DDRA: Data Direction Register A
     case VIA_REG_DDRA:
       return via2__DDRA;
 
     // T1C-L: T1 Low-Order Counter
     case VIA_REG_T1_C_LO:
       // clear T1 interrupt flag
       via2__IFR &= ~VIA_IER_T1;
       // read T1 low order counter
       return via2__timer1Counter & 0xff;
 
     // T1C-H: T1 High-Order Counter
     case VIA_REG_T1_C_HI:
       // read T1 high order counter
       return via2__timer1Counter >> 8;
 
     // T1L-L: T1 Low-Order Latches
     case VIA_REG_T1_L_LO:
       // read T1 low order latch
       return via2__timer1Latch & 0xff;
 
     // T1L-H: T1 High-Order Latches
     case VIA_REG_T1_L_HI:
       // read T1 high order latch
       return via2__timer1Latch >> 8;
 
     // T2C-L: T2 Low-Order Counter
     case VIA_REG_T2_C_LO:
       // clear T2 interrupt flag
       via2__IFR &= ~VIA_IER_T2;
       // read T2 low order counter
       return via2__timer2Counter & 0xff;
 
     // T2C-H: T2 High-Order Counter
     case VIA_REG_T2_C_HI:
       // read T2 high order counter
       return via2__timer2Counter >> 8;
 
     // SR: Shift Register
     case VIA_REG_SR:
       return via2__SR;
 
     // ACR: Auxiliary Control Register
     case VIA_REG_ACR:
       return via2__ACR;
 
     // PCR: Peripheral Control Register
     case VIA_REG_PCR:
       return via2__PCR;
 
     // IFR: Interrupt Flag Register
     case VIA_REG_IFR:
       return via2__IFR | (via2__IFR & via2__IER ? 0x80 : 0);
 
     // IER: Interrupt Enable Register
     case VIA_REG_IER:
       return via2__IER | 0x80;
 
   }
   return 0;
}
 
 
 // ret. true on interrupt
bool via2_tick(int cycles)
 {
   #if DEBUG6522
   via2__tick += cycles;
   #endif
 
   // handle Timer 1
   via2__timer1Counter -= cycles;
   if (via2__timer1Counter <= 0) {
     if (via2__ACR & VIA_ACR_T1_FREERUN) {
       // free run, reload from latch
       via2__timer1Counter += (via2__timer1Latch - 1) + 3;  // +2 delay before next start
       via2__IFR |= VIA_IER_T1;  // set interrupt flag
     } else if (!via2__timer1Triggered) {
       // one shot
       via2__timer1Counter += 0xFFFF;
       via2__timer1Triggered = true;
       via2__IFR |= VIA_IER_T1;  // set interrupt flag
     } else
       via2__timer1Counter = (uint16_t)via2__timer1Counter;  // restart from <0xffff
   }
 
   // handle Timer 2
   if ((via2__ACR & VIA_ACR_T2_COUNTPULSES) == 0) {
     via2__timer2Counter -= cycles;
     if (via2__timer2Counter <= 0 && !via2__timer2Triggered) {
       via2__timer2Counter += 0xFFFF;
       via2__timer2Triggered = true;
       via2__IFR |= VIA_IER_T2;  // set interrupt flag
     }
   }
 
   // handle CA1 (RESTORE key)
   if (via2__CA1 != via2__CA1_prev) {
     // (interrupt on low->high transition) OR (interrupt on high->low transition)
     if (((via2__PCR & 1) && via2__CA1) || (!(via2__PCR & 1) && !via2__CA1)) {
       via2__IFR |= VIA_IER_CA1;
     }
     via2__CA1_prev = via2__CA1;  // set interrupt flag
   }
 
   // handle CB1
   if (via2__CB1 != via2__CB1_prev) {
     // (interrupt on low->high transition) OR (interrupt on high->low transition)
     if (((via2__PCR & 0x10) && via2__CB1) || (!(via2__PCR & 0x10) && !via2__CB1)) {
       via2__IFR |= VIA_IER_CB1;  // set interrupt flag
     }
     via2__CB1_prev = via2__CB1;
   }
 
   return via2__IER & via2__IFR & 0x7f;
}

uint8_t via2_PA()
{
  return via2__PA; 
}

uint8_t via2_PB()
{ 
  return via2__PB; 
}

uint8_t via2_CA1()
{ 
  return via2__CA1; 
}

void via2_setCA1(int value)
{ 
  via2__CA1_prev = via2__CA1; 
  via2__CA1 = value; 
}
 
uint8_t via2_CA2()
{ 
  return via2__CA2; 
}

void via2_setCA2(int value)
{ 
  via2__CA2_prev = via2__CA2; 
  via2__CA2 = value; 
}

uint8_t via2_CB1()
{ 
  return via2__CB1; 
}

void via2_setCB1(int value)
{ 
  via2__CB1_prev = via2__CB1; 
  via2__CB1 = value; 
}
 
uint8_t via2_CB2()
{ 
  return via2__CB2; 
}

void via2_setCB2(int value)
{ 
  via2__CB2_prev = via2__CB2; 
  via2__CB2 = value; 
}

uint8_t via2_DDRA()
{ 
  return via2__DDRA; 
}
 
uint8_t via2_DDRB()
{ 
  return via2__DDRB; 
}
 
