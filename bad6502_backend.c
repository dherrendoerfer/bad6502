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
#include <string.h>
#include <pthread.h>
#include "cpu/bad65C02.h"

#include "6502asm/test.h"

#ifdef FAKE
#define reset65C02 reset6502
#define step65C02 step6502
#define read65C02 read6502
#define write65C02 write6502
#endif

// Memory layout 
volatile unsigned char mem[0x10000];
volatile unsigned char *page[256];
volatile unsigned char page_type[256];

// Threads
pthread_t IOthread, CPUthread, VIDthread;

volatile uint8_t runme = 1;

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

// IO Thread (slightly async)
volatile uint16_t io_mbox[256];
volatile uint8_t updateIO_ready = 0;
void *updateIO()
{
  uint8_t box_pos = 0;
  uint16_t addr;
  dup(1);
  dup(2);
  updateIO_ready = 1;

  while (runme) {
    if (io_mbox[box_pos]) {
      addr = io_mbox[box_pos];
      if (addr == 0xE000) {
        fprintf(stdout,"%c",mem[addr]);
      }
      io_mbox[box_pos++] = 0;
    }
  }
}

// CPU thread (sync)
volatile uint32_t run_state=3;
void *run6502()
{
  reset65C02();

  run_state = 0;
  while (runme) {
    while(!run_state);
    step65C02();
#ifdef FAKE
    usleep(1);
//    extern volatile uint16_t pc;
//    printf("0x%04x\n",pc);
#endif
    run_state--;
  }
}

// Video thread (mostly async)
volatile uint32_t vid_state=3;
void *videoOut()
{
  vid_state=0;
  while(runme);
  // Implement me !

}

// required function 
uint8_t read65C02(uint16_t address)
{
  return mem[address];
}

volatile uint8_t iobox = 0;

// required function 
void write65C02(uint16_t address, uint8_t value)
{
  uint8_t page = address>>8;
  uint8_t type = page_type[page];

  if (type == 3) //Only write to RAM/IO
    return;

  mem[address]=value;

  if (type == 2) { //Notify HW (thread)
    io_mbox[iobox++]=address;
  }
}

// Main prog
int main(int argc, char **argv)
{
  int addr=0;
  long g;

  if (pthread_create(&IOthread, NULL, updateIO, NULL)) {
    printf("thread create failed\n");
    exit(-1);
  }
  while(!updateIO_ready);
  printf("IO Thread running\n");

  if (pthread_create(&CPUthread, NULL, run6502, NULL)) {
    printf("thread create failed\n");
    exit(-1);
  }
  while(run_state);
  printf("CPU Thread running\n");
  
  if (pthread_create(&VIDthread, NULL, videoOut, NULL)) {
    printf("thread create failed\n");
    exit(-1);
  }
  while(vid_state);
  printf("VIDEO Thread running\n");

  // Set up pages and memory (1st go: all RAM)
  for (g=0; g<256; g++) {
    page[g]=&mem[256*g];
    page_type[g]=1; //RAM
  }

  // Page 1 -> IO
  page_type[0xE0]=2; //IO

  // Install ROM/vectors
  install_reset_vect(0x1000);
  install_irq_vect(0x1000);

  // Install test bin
  for (g=0; g<test_len; g++)
    mem[0x1000+g]=test[g];


  printf("-------- START --------\n");

  // Run 1Million cycles
  for (g=0; g<1000; g++) {
    while(run_state);
    run_state=10000;
  }

  runme=0;

  printf("Stopping VIDEO thread\n");
  pthread_join(VIDthread,NULL);
  printf("Stopping IO thread\n");
  pthread_join(IOthread,NULL);
  printf("Stopping CPU thread\n");
  pthread_join(CPUthread,NULL);

  return 0;
}
