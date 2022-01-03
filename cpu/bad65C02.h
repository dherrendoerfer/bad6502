#ifndef _REAL65C02_H_
#define _REAL65C02_H_

#include <stdint.h>

extern void reset65C02();
extern void step65C02();
extern void exec65C02(uint32_t tickcount);
extern void irq65C02();
extern uint32_t clockticks65C02;

#endif
