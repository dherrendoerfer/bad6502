
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -funroll-loops -Wall -I.. -I/usr/include/SDL2 #-DDEBUG6522 #-DDEBUG -DDEBUGDELAY=500000

OBJS = keyboard.o via6522_1.o via6522_2.o

all:  $(OBJS)
	$(CC) -O3 -o bad6502_backend bad6502_backend.c $(OBJS) ../cpu/bad65C02.o -lpthread -I/usr/include/SDL2 -lSDL2 -I.. -DASYNCIO

fake: $(OBJS)
	$(CC) -DFAKE -O3 -o bad6502_backend bad6502_backend.c $(OBJS) ../cpu/fake6502.o -lpthread -I/usr/include/SDL2 -lSDL2 -I.. #-DASYNCIO


clean:
	rm -f *.o bad6502_backend 
