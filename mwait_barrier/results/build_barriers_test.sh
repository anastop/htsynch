#!/bin/bash
make -f Makefile.xeon OTHER_FLAGS=-DMWMON clean barriers
mv barriers barriers.mwmon

make -f Makefile.xeon OTHER_FLAGS=-DSPINHALT clean barriers
mv barriers barriers.spinhalt

make -f Makefile.xeon OTHER_FLAGS=-DSPINPAUSE clean barriers
mv barriers barriers.spinpause

make -f Makefile.xeon OTHER_FLAGS=-DPTHREAD clean barriers
mv barriers barriers.pthread

