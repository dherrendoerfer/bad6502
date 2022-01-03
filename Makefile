
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -funroll-loops -Wall 

OBJS_CPU = cpu/bad65C02.o


all:  $(OBJS_CPU) 
	$(CC) -O3 -o bad6502_backend bad6502_backend.c $(OBJS_CPU) -lrt

clean:
	rm -f cpu/*.o bad6502_backend
