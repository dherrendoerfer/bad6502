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

#define BCM2708_PERI_BASE        0x3F000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "bad65C02.h"

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

#define GET_ADDR (*(gpio+13)>>8)&0xFFFF
#define GET_DATA (*(gpio+13))&0xFF
#define GET_RW (*(gpio+13))&(1<<24)
#define GET_ALL_GPIO (*(gpio+13))

// Local helpers 
void setup_io();
void nsleep(int);
void one_clock();

// ndelay is a define so the compiler can unroll it ;-)
#define ndelay(x) for(int i=0;i<x;i++)asm("nop");

// Defined elsewehere (hopefully)
extern uint8_t read65C02(uint16_t address);
extern void write65C02(uint16_t address, uint8_t value);
extern void update65C02();

// Tick counter
volatile uint32_t clockticks65C02;

volatile uint8_t proc_init_done = 0;

// For fake6502 compatibility (x16-emulator)
uint16_t pc;
uint8_t sp,a,x,y,status;

// Defined for speed
#define _65C02_gpio_data_r 0x0
#define _65C02_gpio_data_w 0x249249

volatile uint8_t _65C02irq = 0;
volatile uint8_t _65C02reset = 0;

void init65C02()
{
  int g;
#ifdef DEBUG
  printf("Init65C05\n");
#endif

  // Set up gpi pointer for direct register access
  setup_io();

  // Set GPIO pins 0-7 to output
  for (g=0; g<=7; g++) {
    INP_GPIO(g); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(g);
  }

  // Set GPIO pins 8-23 to input
  for (g=8; g<=23; g++) {
    INP_GPIO(g);
  }

  // Set GPIO pins 24 to input (!RW)
  INP_GPIO(24);

  // Set GPIO pins 25 to output (CLOCK)
  INP_GPIO(25);
  OUT_GPIO(25);

  // Set GPIO pins 26 to output (!RESET)
  INP_GPIO(26);
  OUT_GPIO(26);
  GPIO_SET = 1<<26; //release !RESET

  // Set GPIO pins 27 to output (!IRQ)
  INP_GPIO(27);
  OUT_GPIO(27);
  GPIO_SET = 1<<27; //release !IRQ
 
  proc_init_done = 1; 
}

uint32_t bus_data = 0;
uint32_t bus_rw = 0;
uint32_t bus_addr = 0;

void step65C02() 
{
  //TODO: The delays need to be optimized (scoped)
  //If you think it's odd that we'reading io twice ... it's a pi bug.

  // Clock cycle start (clock goes low)
  GPIO_CLR = 1<<25;
  GPIO_CLR = 1<<25;

  // wait the setup time to read the next addr
  ndelay(10);
  // STEP callback (in cpu thread)
  update65C02();

  // Address and RW stable
  bus_addr = GET_ADDR;
  bus_addr = GET_ADDR;
  bus_rw = GET_RW;
  bus_rw = GET_RW;

  // If there's real hardware at this address then dont
  // do anything to drive the data lines, keep reading

  //bus_rw = _isHW(&bus_addr,&bus_rw);

  // 2nd part of clock cycle (clock goes high)
  GPIO_SET = 1<<25;
  GPIO_SET = 1<<25;

  // Set DATA to INP or OUT based on RW
  *(gpio) = _65C02_gpio_data_r;
  if (bus_rw) {
    *(gpio) = _65C02_gpio_data_w;
  }

  if (bus_rw) {
    //write to 65C02
    GPIO_SET=0xFF;
    GPIO_CLR=~read65C02(bus_addr)&0xFF;
    ndelay(50);
  }
  else {
    ndelay(10);
    //read from 5C02
    bus_data=GET_DATA;
    bus_data=GET_DATA;
    write65C02(bus_addr, bus_data);
  }

  // reset HW signal lines
  if (_65C02irq) {
    GPIO_SET = 1<<27; //release !IRQ
    _65C02irq = 0;
  }

  if (_65C02reset) {
    GPIO_SET = 1<<26; //release !RESET
    _65C02reset = 0;
  }

  clockticks65C02++;
  pc=bus_addr;

#ifdef DEBUG
#ifndef DEBUGDELAY
#define DEBUGDELAY 20000
#endif
  if (usleep(DEBUGDELAY))
    return;

  printf("A=0x%04x,",bus_addr);

  for (int g=15; g>=0; g--) {
    if (bus_addr&(1<<g))
      printf("1");
    else
      printf("0");
  }

  printf(", ");

  if (bus_rw)
    printf("r <- M=0x%02x", read65C02(bus_addr));
  else
    printf("w -> D=0x%02x", bus_data);

  printf("\n");
#endif
}

void exec65C02(uint32_t tickcount)
{
  uint32_t g;
  for (g=0; g<tickcount; g++)
    step65C02();  
}

void reset65C02()
{
#ifdef DEBUG
  printf("Reset65C05\n");
#endif

  if (!proc_init_done)
    init65C02();

  // Perform 6502 reset
  GPIO_CLR = 1<<26; //set !RESET
  _65C02reset = 1;
  clockticks65C02 = 0;
}

void irq65C02()
{
#ifdef DEBUG
  printf("IRQ65C05\n");
#endif

  // Perform 6502 irq
  GPIO_CLR = 1<<27; //set !IRQ
  _65C02irq=1;
}

//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL, 
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      mem_fd,
      GPIO_BASE
   );
   /* close fd */
   close(mem_fd); 

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   /* store map pointer */
   gpio = (volatile unsigned *)gpio_map;
}

struct timespec _65C02_ts;

// nsleep: sleep for some nanoseconds,
// But beware! this might lead to a reschedule, and the 
// sheduler is milliseconds based. We don't want to sleep here!
void nsleep(int nanos)
{
  struct timespec ts;
  int res;

  ts.tv_sec = 0;
  ts.tv_nsec = nanos;

  do {
    res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);
}

// one_clock: do one clock cycle (ignore everything)
void one_clock()
{
  GPIO_CLR = 1<<25;
  nsleep(500);

  GPIO_SET = 1<<25;
  nsleep(500);
}


