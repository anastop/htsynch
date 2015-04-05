#!/bin/bash
make -f Makefile.xeon OTHER_FLAGS=-DMWMON clean barriers_2packages
mv barriers_2packages barriers_2packages.mwmon

make -f Makefile.xeon OTHER_FLAGS=-DSPINHALT clean barriers_2packages
mv barriers_2packages barriers_2packages.spinhalt

make -f Makefile.xeon OTHER_FLAGS=-DSPINPAUSE clean barriers_2packages
mv barriers_2packages barriers_2packages.spinpause

make -f Makefile.xeon OTHER_FLAGS=-DPTHREAD clean barriers_2packages
mv barriers_2packages barriers_2packages.pthread

