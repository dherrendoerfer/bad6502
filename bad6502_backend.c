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
#include <SDL.h>
#include "cpu/bad65C02.h"

#include "6502asm/test.h"

// VIC-20 VIC implementation for test
#include "./vic20/roms/characters.901460-03.h"

#ifdef FAKE
#define reset65C02 reset6502
#define step65C02 step6502
#define read65C02 read6502
#define write65C02 write6502
#define clockticks65C02 clockticks6502

volatile extern uint32_t clockticks6502;
#endif

// ndelay is a define so the compiler can unroll it ;-)
#define ndelay(x) for(int i=0;i<x;i++)asm("nop");

// Memory layout 
volatile static uint8_t mem[0x10000];
volatile static uint8_t *page[256];
volatile static uint8_t page_type[256];

// Threads
pthread_t IOthread, CPUthread, VIDthread;

volatile uint8_t runme = 1;

// install_reset_vector: set target for reset
inline static void install_reset_vect(unsigned int vect) 
{
  mem[0xFFFC]=(vect&0xff);
  mem[0xFFFD]=(vect&0xff00)>>8;
}

// install_irq_vector: set target for reset
inline static void install_irq_vect(unsigned int vect) 
{
  mem[0xFFFE]=(vect&0xff);
  mem[0xFFFF]=(vect&0xff00)>>8;
}

// install_nmi_vector: set target for reset
inline static void install_nmi_vect(unsigned int vect) 
{
  mem[0xFFFA]=(vect&0xff);
  mem[0xFFFB]=(vect&0xff00)>>8;
}

// IO Thread (slightly async)
volatile uint16_t io_mbox[256];
volatile uint8_t mbox_data[256];
volatile uint8_t updateIO_ready = 0;

void *updateIO()
{
  uint8_t box_pos = 0;
  uint16_t addr;
  uint32_t old_ticks = clockticks65C02;
  dup(1);
  dup(2);
  updateIO_ready = 1;

  while (runme) {
    while (clockticks65C02 == old_ticks);
    if (io_mbox[box_pos]) {
      addr = io_mbox[box_pos];
      if (addr == 0xE000) {
        fprintf(stdout,"%c",mbox_data[box_pos]);
      }
      io_mbox[box_pos++] = 0;
    }
    old_ticks++;
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
    ndelay(800);
//    extern volatile uint16_t pc;
//    printf("0x%04x\n",pc);
#endif
    run_state--;
  }
}


// Video defines
// visible area we're drawing
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define SCREEN_RAM_OFFSET 0x00000

#define LSHORTCUT_KEY SDL_SCANCODE_LCTRL
#define RSHORTCUT_KEY SDL_SCANCODE_RCTRL

// When rendering a layer line, we can amortize some of the cost by calculating multiple pixels at a time.
#define LAYER_PIXELS_PER_ITERATION 8


static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;
int window_scale = 1;
char *scale_quality = "best";
static uint16_t width = SCREEN_WIDTH;
static uint16_t height = SCREEN_HEIGHT;

static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

// Video_init
void video_init(int window_scale, char *quality)
{
  uint32_t window_flags = SDL_WINDOW_ALLOW_HIGHDPI;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, quality);
  SDL_CreateWindowAndRenderer(SCREEN_WIDTH * window_scale, SCREEN_HEIGHT * window_scale, window_flags, &window, &renderer);
  SDL_SetWindowResizable(window, 1);
  SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  sdlTexture = SDL_CreateTexture(renderer,
					SDL_PIXELFORMAT_RGB888,
					SDL_TEXTUREACCESS_STREAMING,
					SCREEN_WIDTH, SCREEN_HEIGHT);

  SDL_SetWindowTitle(window, "bad65C02");

  SDL_ShowCursor(SDL_DISABLE);

  if (sdlTexture)
    printf("Video setup success!\n");
}

void video_cleanup()
{
  printf("VIDEO cleanup\n");
  SDL_DestroyTexture( sdlTexture );
  SDL_DestroyRenderer( renderer );
  SDL_DestroyWindow( window );
  SDL_Quit();
}

#define VID_MEMSTART 0x1000
#define CHAR_ROMSTART 0x8000

void draw_console_toFB()
{
  uint16_t g,h,i,j;
  uint8_t buf, c_rom;

  for (g=0; g<23; g++) {
    for (h=0; h<22; h++) {
      //
      buf = mem[VID_MEMSTART+(22*g)+h];

      for (i=0; i<8; i++) {
        c_rom=mem[CHAR_ROMSTART+(8*buf)+i];
       
	uint32_t *fb = (uint32_t*)framebuffer;

        for (j=0; j<8; j++) {
	  if (c_rom&(1<<j)) {
	    //setpixel
	    fb[((10+g*8)+i)*SCREEN_WIDTH+70+(h*8)+8-j] = 0x00;
	  }
	  else {
            //clearpixel 
	    fb[((10+g*8)+i)*SCREEN_WIDTH+70+(h*8)+8-j] = 0x00ffffff;
	  }
	}
      }
    }
  }
}

// Video thread (mostly async)
volatile uint32_t vid_state=3;
void *videoOut()
{
  uint32_t old_ticks = clockticks65C02;

  // Initialize VIDEO Window/Surface
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  video_init(window_scale,scale_quality);

  vid_state=0;
  while(runme) {
    SDL_Event ev;
    while( SDL_PollEvent( &ev ) ) {
      switch(ev.type)
      {
        case SDL_QUIT:
        {
          runme = 0;
	  break;
        }

        case SDL_KEYDOWN:
	{
	  if (ev.key.keysym.sym == SDLK_ESCAPE  )
            runme = 0;
	}

        case SDL_WINDOWEVENT:
        {
          switch(ev.window.event)
          {
            case SDL_WINDOWEVENT_SIZE_CHANGED:  {
              width = ev.window.data1;
              height = ev.window.data2;
              break;
            }

            case SDL_WINDOWEVENT_CLOSE:  {
              ev.type = SDL_QUIT;
              SDL_PushEvent(&ev);
              break;
            }
	  }
	}
      }

    }

    if (clockticks65C02 > old_ticks+20000) {
      // Do graphics
      //

      mem[VID_MEMSTART]=clockticks65C02&0xff;

      draw_console_toFB();

      SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);

      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);

      SDL_RenderPresent(renderer);

      old_ticks = clockticks65C02;
    }
  }

  // Destroy VIDEO Window
  video_cleanup();
}

// required function 
void update65C02()
{
  // Be quick in here. this function should take 120ns constantly
}

uint8_t read65C02(uint16_t address)
{
  return mem[address];
}

uint8_t iobox = 0;

// required function 
void write65C02(uint16_t address, uint8_t value)
{
  uint8_t page = address>>8;
  uint8_t type = page_type[page];

  if (type == 3) //Only write to RAM/IO
    return;

  mem[address]=value;

  if (type == 2) { //Notify HW (thread)
    mbox_data[iobox]=value;
    io_mbox[iobox++]=address;
    io_mbox[iobox]=0;
  }
}

uint8_t int_active = 0;
void sig_handler(int signum){
  //Return type of the handler function should be void
  printf("\nCAUGHT SIGINT press again to kill\n");
  if (int_active)
    exit(2);
  int_active=1;
  runme=0;
}

// Main prog
int main(int argc, char **argv)
{
  int addr=0;
  long g;

  signal(SIGINT,sig_handler);

#ifdef FAKE
  printf("Running in FAKE mode !!!!!!!\n");
#endif

  // Set up pages and memory (1st go: all RAM)
  for (g=0; g<256; g++) {
    page[g]=&mem[256*g];
    page_type[g]=1; //RAM
  }

  for (g=0; g<charROM_len; g++) {
    mem[CHAR_ROMSTART+g] = charROM[g];
    page_type[(CHAR_ROMSTART+g)>>8] = 3; //ROM
  }

  // Page 1 -> IO
  page_type[0xE0]=2; //IO

  // Page FF -> ROM
  page_type[0xFF]=3; //IO

  // Install ROM/vectors
  install_reset_vect(0x1000);
  install_irq_vect(0x2000);
#ifdef FAKE
  install_irq_vect(0x1000);
#endif

  // Install test bin
  for (g=0; g<test_len; g++)
    mem[0x1000+g]=test[g];

  //Setup threads
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


  printf("-------- START --------\n");

  // Run 1Million cycles
  for (g=0; g<1000; g++) {
    while(run_state);
    run_state=1000;
    if (!runme)
      break;
  }

  printf("\n\nExecution stopped.");

  //
  // Shutdown
  //

  runme=0;
  usleep(100);
  //Unstick threads that are waiting on a clock cycle
  clockticks65C02++;
  run_state=1;

  printf("Stopping VIDEO thread\n");
  pthread_join(VIDthread,NULL);
  printf("Stopping IO thread\n");
  pthread_join(IOthread,NULL);
  printf("Stopping CPU thread\n");
  pthread_join(CPUthread,NULL);

  usleep(100);
  return 0;
}

