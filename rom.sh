#!/bin/sh

COMBINEDFILE=images/rom.bin

rm -f $COMBINEDFILE

dd conv=notrunc bs=1 if=/home/colin/Workspace/mackerel-68k/firmware/bootloader.bin of=$COMBINEDFILE seek=0 
dd conv=notrunc bs=1 if=images/romfs.img of=$COMBINEDFILE seek=$((0x3000))

minipro -p SST39SF040 -s -w $COMBINEDFILE
