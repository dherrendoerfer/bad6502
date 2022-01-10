
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -funroll-loops -Wall #-DDEBUG

OBJS_CPU = cpu/bad65C02.o
OBJS_FAKE = cpu/fake6502.o

all:  $(OBJS_CPU) 6502asm/test.h
	$(CC) -O3 -o bad6502_backend bad6502_backend.c $(OBJS_CPU) -lpthread

6502asm/test.h: 6502asm/test.asm
	(cd 6502asm; ./build.sh)

fake: $(OBJS_FAKE)
	$(CC) -DFAKE -O3 -o bad6502_backend bad6502_backend.c $(OBJS_FAKE) -lpthread


clean:
	rm -f cpu/*.o 6502asm/test.h bad6502_backend 
