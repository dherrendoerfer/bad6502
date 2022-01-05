# bad6502
Probably just a bad idea, but lets build a 6502 computer with a pi as everything but the CPU
### hardware
Simply put, strap a WDC65C02 to a Raspberry Pi Zero 2W like this: 
- GPIO 0-7 -> DATA 0-7
- GPIO 8-23 -> ADDRESS 0-15
- GPIO 24 -> !RW
- GPIO 25 -> CLOCK
- GPIO 26 -> !RESET
- GPIO 27 -> !IRQ
Add 10k pull-up resistors to these 65C02 pins
- !IRQ
- !RESET
- !NMI
- !SO
- !RDY
### software
There's a cpu subdir in this repo which contains a set of files to start and run the cpu very much like fake6502.  
A backend for some support hardware is there for testing, but more will come over time.
### future plans?
I want to get to the point were theres a PCB kit available that holds the CPU and probably some support hardware. Lets see ...
### license
It's GPLv3, read the License file in the repo.
