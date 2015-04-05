#!/bin/bash
make -f Makefile.xeon OTHER_FLAGS=-DMWMON clean primitives
mv primitives primitives.mwmon

make -f Makefile.xeon OTHER_FLAGS=-DSPINHALT clean primitives
mv primitives primitives.spinhalt

make -f Makefile.xeon OTHER_FLAGS=-DSPINPAUSE clean primitives
mv primitives primitives.spinpause

make -f Makefile.xeon OTHER_FLAGS=-DPTHREAD clean primitives
mv primitives primitives.pthread

