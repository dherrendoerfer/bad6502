 *                                                   *
 * This interface is based on Fake6502.              *
 * Usage:                                            *
 *                                                   *
 * real_65C02 requires you to provide two external   *
 * functions:                                        *
 *                                                   *
 * uint8_t read65C02(uint16_t address)               *
 * void write65C02(uint16_t address, uint8_t value)  *
 *                                                   *
 *****************************************************
 * Useful functions :                                *
 *                                                   *
 * void reset65C2()                                  *
 *   - Call this once before you begin execution.    *
 *                                                   *
 * void exec65C02(uint32_t tickcount)                *
 *   - Execute 6502 code up to the next specified    *
 *     count of clock ticks.                         *
 *                                                   *
 * void step65C02()                                  *
 *   - Execute a single instrution.                  *
 *                                                   *
 * void irq65C02()                                   *
 *   - Trigger a hardware IRQ in the 6502 core.      *
 *                                                   *
 * void nmi65C02()                                   *
 *   - Trigger an NMI in the 6502 core.              *
 *                                                   *
 *****************************************************
 * Useful variables in this emulator:                *
 *                                                   *
 * uint32_t clockticks65C02                          *
 *   - A running total of the emulated cycle count.  *
 *                                                   *
 *****************************************************

PLEASE NOTE THAT THIS PROJECT ONLY WORKS ON A PI ZERO 2W
