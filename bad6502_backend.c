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
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "cpu/bad65C02.h"

// Memory layout 
unsigned char mem[0x10000];
unsigned char *page[256];
unsigned char page_type[256];

// install_reset_vector: set target for reset
inline void install_reset_vect(unsigned int vect) 
{
  mem[0xFFFC]=(vect&0xff);
  mem[0xFFFD]=(vect&0xff00)>>8;
}

// install_irq_vector: set target for reset
inline void install_irq_vect(unsigned int vect) 
{
  mem[0xFFFE]=(vect&0xff);
  mem[0xFFFF]=(vect&0xff00)>>8;
}

// install_nmi_vector: set target for reset
inline void install_nmi_vect(unsigned int vect) 
{
  mem[0xFFFA]=(vect&0xff);
  mem[0xFFFB]=(vect&0xff00)>>8;
}

// required function 
uint8_t read65C02(uint16_t address)
{
  return mem[address];
}

// required function 
void write65C02(uint16_t address, uint8_t value)
{
  if (page_type[address>>8] == 1) //Only write to RAM
    mem[address]=value;
}

int main(int argc, char **argv)
{
  int addr=0;
  long g;

  // Set up pages and memory (1st go: all RAM)
  for (g=0; g<256; g++) {
    page[g]=&mem[256*g];
    page_type[g]=1; //RAM
  }

  // Install ROM/vectors
  install_reset_vect(0x1000);
  install_irq_vect(0x2000);

  reset65C02();

  g=0;

  while (g<1000000) {
    step65C02();
    g++;
  }

  return 0;

} // main

