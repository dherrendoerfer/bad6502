#!/bin/bash

xa test.asm || exit 1
bin2c -i a.o65 -o test.h -a test
rm a.o65
