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
#include "via6522_1.h"
 
 
// VIA (6522 - Versatile Interface Adapter)

// timers
volatile int           via1__timer1Counter;
volatile uint16_t      via1__timer1Latch;
volatile int           via1__timer2Counter;
volatile uint8_t       via1__timer2Latch;     // timer 2 latch is 8 bits
volatile bool          via1__timer1Triggered;
volatile bool          via1__timer2Triggered;
 
// CA1, CA2
volatile uint8_t       via1__CA1;
volatile uint8_t       via1__CA1_prev;
volatile uint8_t       via1__CA2;
volatile uint8_t       via1__CA2_prev;
 
// CB1, CB2
volatile uint8_t       via1__CB1;
volatile uint8_t       via1__CB1_prev;
volatile uint8_t       via1__CB2;
volatile uint8_t       via1__CB2_prev;
 
// PA, PB
volatile uint8_t       via1__DDRA;
volatile uint8_t       via1__DDRB;
volatile uint8_t       via1__PA;     // what actually there is out of 6522 port A
volatile uint8_t       via1__PB;     // what actually there is out of 6522 port B
volatile uint8_t       via1__IRA;    // input register A
volatile uint8_t       via1__IRB;    // input register B
volatile uint8_t       via1__ORA;    // output register A
volatile uint8_t       via1__ORB;    // output register B
 
volatile uint8_t       via1__IFR;
volatile uint8_t       via1__IER;
volatile uint8_t       via1__ACR;
volatile uint8_t       via1__PCR;
volatile uint8_t       via1__SR;
 
#if DEBUG6522
volatile int           via1__tick;
#endif

#if DEBUG6522
static volatile const char * VIAREG2STR[] = { "ORB_IRB", "ORA_IRA", "DDRB", "DDRA", "T1_C_LO", "T1_C_HI", "T1_L_LO", "T1_L_HI", "T2_C_LO", "T2_C_HI", "SR", "ACR", "PCR", "IFR", "IER", "ORA_IRA_NH" };
#endif

void via1_reset()
{
   via1__timer1Counter   = 0x0000;
   via1__timer1Latch     = 0x0000;
   via1__timer2Counter   = 0x0000;
   via1__timer2Latch     = 0x00;
   via1__CA1             = 0;
   via1__CA1_prev        = 0;
   via1__CA2             = 0;
   via1__CA2_prev        = 0;
   via1__CB1             = 0;
   via1__CB1_prev        = 0;
   via1__CB2             = 0;
   via1__CB2_prev        = 0;
   via1__IFR             = 0;
   via1__IER             = 0;
   via1__ACR             = 0;
   via1__timer1Triggered = false;
   via1__timer2Triggered = false;
   via1__DDRA            = 0;
   via1__DDRB            = 0;
   via1__PCR             = 0;
   via1__PA              = 0xff;
   via1__PB              = 0xff;
   via1__SR              = 0;
   via1__IRA             = 0xff;
   via1__IRB             = 0xff;
   via1__ORA             = 0;
   via1__ORB             = 0;
}
 
 
void via1_setPA(int value)
{
   via1__PA = value;
   via1__IRA = via1__PA; // IRA is exactly what there is on port A
}
 
 
void via1_setBitPA(int bit, bool value)
{
   uint8_t newPA = (via1__PA & ~(1 << bit)) | ((int)value << bit);
   via1_setPA(newPA);
}
 
 
void via1_clearBitPA(int bit)
{
   uint8_t mask = (1 << bit);
   if (via1__DDRA & mask) {
     // pin configured as output, set value of ORA
     via1_setBitPA(bit, via1__ORA & mask);
   } else {
     // pin configured as input, pull it up
     via1_setBitPA(bit, true);
   }
}
 
 
void via1_setPB(int value)
{
   via1__PB = value;
   via1__IRB = (via1__PB & ~via1__DDRB) | (via1__ORB & via1__DDRB);  // IRB contains PB for inputs, and ORB for outputs
}
 
 
void via1_setBitPB(int bit, bool value)
{
   uint8_t newPB = (via1__PB & ~(1 << bit)) | ((int)value << bit);
   via1_setPB(newPB);
}
 
 
void via1_clearBitPB(int bit)
{
   uint8_t mask = (1 << bit);
   if (via1__DDRB & mask) {
     // pin configured as output, set value of ORB
     via1_setBitPB(bit, via1__ORB & mask);
   } else {
     // pin configured as input, pull it up
     via1_setBitPB(bit, true);
   }
}
 
 
// reg must be 0..0x0f (not checked)
void via1_writeReg(int reg, int value)
{
   #if DEBUG6522
   printf("%d",reg);
   printf("\ntick %d VIA 1 writeReg(%s, 0x%02x)\n", via1__tick, VIAREG2STR[reg], value);
   #endif
 
   switch (reg) {
 
     // ORB: Output Register B
     case VIA_REG_ORB_IRB:
       via1__ORB = value;
       via1__PB  = (via1__ORB & via1__DDRB) | (via1__PB & ~via1__DDRB);
       via1__IRB = (via1__PB & ~via1__DDRB) | (via1__ORB & via1__DDRB);  // IRB contains PB for inputs, and ORB for outputs
       // clear CB1 and CB2 interrupt flags
       via1__IFR &= ~VIA_IER_CB1;
       via1__IFR &= ~VIA_IER_CB2;
       break;
 
     // ORA: Output Register A
     case VIA_REG_ORA_IRA:
       // clear CA1 and CA2 interrupt flags
       via1__IFR &= ~VIA_IER_CA1;
       via1__IFR &= ~VIA_IER_CA2;
       // not break!
     // ORA: Output Register A - no Handshake
     case VIA_REG_ORA_IRA_NH:
       via1__ORA = value;
       via1__PA  = (via1__ORA & via1__DDRA) | (via1__PA & ~via1__DDRA);
       via1__IRA = via1__PA;                                   // IRA is exactly what there is on port A
       break;
 
     // DDRB: Data Direction Register B
     case VIA_REG_DDRB:
       via1__DDRB = value;
       via1__PB   = (via1__ORB & via1__DDRB) | (via1__PB & ~via1__DDRB);   // refresh Port B status
       via1__IRB  = (via1__PB & ~via1__DDRB) | (via1__ORB & via1__DDRB);   // IRB contains PB for inputs, and ORB for outputs
       break;
 
     // DDRA: Data Direction Register A
     case VIA_REG_DDRA:
       via1__DDRA = value;
       via1__PA   = (via1__ORA & via1__DDRA) | (via1__PA & ~via1__DDRA);   // refresh Port A status
       via1__IRA  = via1__PA;                                  // IRA is exactly what there is on port A
       break;
 
     // T1C-L: T1 Low-Order Latches
     case VIA_REG_T1_C_LO:
       via1__timer1Latch = (via1__timer1Latch & 0xff00) | value;
       break;
 
     // T1C-H: T1 High-Order Counter
     case VIA_REG_T1_C_HI:
       via1__timer1Latch = (via1__timer1Latch & 0x00ff) | (value << 8);
       // timer1: write into high order counter
       // timer1: transfer low order latch into low order counter
       via1__timer1Counter = (via1__timer1Latch & 0x00ff) | (value << 8);
       // clear T1 interrupt flag
       via1__IFR &= ~VIA_IER_T1;
       via1__timer1Triggered = false;
       break;
 
     // T1L-L: T1 Low-Order Latches
     case VIA_REG_T1_L_LO:
       via1__timer1Latch = (via1__timer1Latch & 0xff00) | value;
       break;
 
     // T1L-H: T1 High-Order Latches
     case VIA_REG_T1_L_HI:
       via1__timer1Latch = (via1__timer1Latch & 0x00ff) | (value << 8);
       // clear T1 interrupt flag
       via1__IFR &= ~VIA_IER_T1;
       break;
 
     // T2C-L: T2 Low-Order Latches
     case VIA_REG_T2_C_LO:
       via1__timer2Latch = value;
       break;
 
     // T2C-H: T2 High-Order Counter
     case VIA_REG_T2_C_HI:
       // timer2: copy low order latch into low order counter
       via1__timer2Counter = (value << 8) | via1__timer2Latch;
       // clear T2 interrupt flag
       via1__IFR &= ~VIA_IER_T2;
       via1__timer2Triggered = false;
       break;
 
     // SR: Shift Register
     case VIA_REG_SR:
       via1__SR = value;
       break;
 
     // ACR: Auxliary Control Register
     case VIA_REG_ACR:
       via1__ACR = value;
       break;
 
     // PCR: Peripheral Control Register
     case VIA_REG_PCR:
     {
       via1__PCR = value;
       // CA2 control
       switch ((via1__PCR >> 1) & 0b111) {
         case 0b110:
           // manual output - low
           via1__CA2 = 0;
           break;
         case 0b111:
           // manual output - high
           via1__CA2 = 1;
           break;
         default:
           break;
       }
       // CB2 control
       switch ((via1__PCR >> 5) & 0b111) {
         case 0b110:
           // manual output - low
           via1__CB2 = 0;
           break;
         case 0b111:
           // manual output - high
           via1__CB2 = 1;
           break;
         default:
           break;
       }
       break;
     }
 
     // IFR: Interrupt Flag Register
     case VIA_REG_IFR:
       // reset each bit at 1
       via1__IFR &= ~value & 0x7f;
       break;
 
     // IER: Interrupt Enable Register
     case VIA_REG_IER:
       if (value & VIA_IER_CTRL) {
         // set 0..6 bits
         via1__IER |= value & 0x7f;
       } else {
         // reset 0..6 bits
         via1__IER &= ~value & 0x7f;
       }
       break;
   };
}
 
 
 // reg must be 0..0x0f (not checked)
int via1_readReg(int reg)
{
   #if DEBUG6522
   printf("%d",reg);
   printf("\ntick %d VIA 1 readReg(%s)\n", via1__tick, VIAREG2STR[reg]);
   #endif
 
   switch (reg) {
 
     // IRB: Input Register B
     case VIA_REG_ORB_IRB:
       // clear CB1 and CB2 interrupt flags
       via1__IFR &= ~VIA_IER_CB1;
       via1__IFR &= ~VIA_IER_CB2;
       // get updated PB status
       return via1__IRB;
 
     // IRA: Input Register A
     case VIA_REG_ORA_IRA:
       // clear CA1 and CA2 interrupt flags
       via1__IFR &= ~VIA_IER_CA1;
       via1__IFR &= ~VIA_IER_CA2;
     // IRA: Input Register A - no handshake
     case VIA_REG_ORA_IRA_NH:
       // get updated PA status
       return via1__IRA;
 
     // DDRB: Data Direction Register B
     case VIA_REG_DDRB:
       return via1__DDRB;
 
     // DDRA: Data Direction Register A
     case VIA_REG_DDRA:
       return via1__DDRA;
 
     // T1C-L: T1 Low-Order Counter
     case VIA_REG_T1_C_LO:
       // clear T1 interrupt flag
       via1__IFR &= ~VIA_IER_T1;
       // read T1 low order counter
       return via1__timer1Counter & 0xff;
 
     // T1C-H: T1 High-Order Counter
     case VIA_REG_T1_C_HI:
       // read T1 high order counter
       return via1__timer1Counter >> 8;
 
     // T1L-L: T1 Low-Order Latches
     case VIA_REG_T1_L_LO:
       // read T1 low order latch
       return via1__timer1Latch & 0xff;
 
     // T1L-H: T1 High-Order Latches
     case VIA_REG_T1_L_HI:
       // read T1 high order latch
       return via1__timer1Latch >> 8;
 
     // T2C-L: T2 Low-Order Counter
     case VIA_REG_T2_C_LO:
       // clear T2 interrupt flag
       via1__IFR &= ~VIA_IER_T2;
       // read T2 low order counter
       return via1__timer2Counter & 0xff;
 
     // T2C-H: T2 High-Order Counter
     case VIA_REG_T2_C_HI:
       // read T2 high order counter
       return via1__timer2Counter >> 8;
 
     // SR: Shift Register
     case VIA_REG_SR:
       return via1__SR;
 
     // ACR: Auxiliary Control Register
     case VIA_REG_ACR:
       return via1__ACR;
 
     // PCR: Peripheral Control Register
     case VIA_REG_PCR:
       return via1__PCR;
 
     // IFR: Interrupt Flag Register
     case VIA_REG_IFR:
       return via1__IFR | (via1__IFR & via1__IER ? 0x80 : 0);
 
     // IER: Interrupt Enable Register
     case VIA_REG_IER:
       return via1__IER | 0x80;
 
   }
   return 0;
}
 
 
 // ret. true on interrupt
bool via1_tick(int cycles)
 {
   #if DEBUG6522
   via1__tick += cycles;
   #endif
 
   // handle Timer 1
   via1__timer1Counter -= cycles;
   if (via1__timer1Counter <= 0) {
     if (via1__ACR & VIA_ACR_T1_FREERUN) {
       // free run, reload from latch
       via1__timer1Counter += (via1__timer1Latch - 1) + 3;  // +2 delay before next start
       via1__IFR |= VIA_IER_T1;  // set interrupt flag
     } else if (!via1__timer1Triggered) {
       // one shot
       via1__timer1Counter += 0xFFFF;
       via1__timer1Triggered = true;
       via1__IFR |= VIA_IER_T1;  // set interrupt flag
     } else
       via1__timer1Counter = (uint16_t)via1__timer1Counter;  // restart from <0xffff
   }
 
   // handle Timer 2
   if ((via1__ACR & VIA_ACR_T2_COUNTPULSES) == 0) {
     via1__timer2Counter -= cycles;
     if (via1__timer2Counter <= 0 && !via1__timer2Triggered) {
       via1__timer2Counter += 0xFFFF;
       via1__timer2Triggered = true;
       via1__IFR |= VIA_IER_T2;  // set interrupt flag
     }
   }
 
   // handle CA1 (RESTORE key)
   if (via1__CA1 != via1__CA1_prev) {
     // (interrupt on low->high transition) OR (interrupt on high->low transition)
     if (((via1__PCR & 1) && via1__CA1) || (!(via1__PCR & 1) && !via1__CA1)) {
       via1__IFR |= VIA_IER_CA1;
     }
     via1__CA1_prev = via1__CA1;  // set interrupt flag
   }
 
   // handle CB1
   if (via1__CB1 != via1__CB1_prev) {
     // (interrupt on low->high transition) OR (interrupt on high->low transition)
     if (((via1__PCR & 0x10) && via1__CB1) || (!(via1__PCR & 0x10) && !via1__CB1)) {
       via1__IFR |= VIA_IER_CB1;  // set interrupt flag
     }
     via1__CB1_prev = via1__CB1;
   }
 
   return via1__IER & via1__IFR & 0x7f;
}

 
uint8_t via1_PA() 
{ 
  return via1__PA; 
}

uint8_t via1_PB()
{ 
  return via1__PB; 
}

uint8_t via1_CA1()
{ 
  return via1__CA1; 
}

void via1_setCA1(int value)
{ 
  via1__CA1_prev = via1__CA1; 
  via1__CA1 = value; 
}
 
uint8_t via1_CA2()
{ 
  return via1__CA2; 
}

void via1_setCA2(int value)
{ 
  via1__CA2_prev = via1__CA2; 
  via1__CA2 = value; 
}

uint8_t via1_CB1()
{ 
  return via1__CB1; 
}

void via1_setCB1(int value)
{ 
  via1__CB1_prev = via1__CB1; 
  via1__CB1 = value; 
}
 
uint8_t via1_CB2()
{ 
  return via1__CB2; 
}

void via1_setCB2(int value)
{ 
  via1__CB2_prev = via1__CB2; 
  via1__CB2 = value; 
}
 
uint8_t via1_DDRA()
{ 
  return via1__DDRA; 
}
 
uint8_t via1_DDRB()
{ 
  return via1__DDRB; 
}
 
