#!/bin/bash
make clean
make dirs
make elf
cp createimage build/
cd build && ./createimage --extended bootblock main && cd ..