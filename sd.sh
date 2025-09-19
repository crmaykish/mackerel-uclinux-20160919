#!/bin/sh

# This script creates a "bootable" SD card for Mackerel-08
# First, the file size of the linux image is written to the first bytes of the SD card
# Then, the Linux image (kernel + ROMfs) is written with a one block offset (it starts
# at block 1, not 0)
# The bootloader can read the first block of the SD card to determine the image size
# and then read enough blocks to load the whole Linux image into RAM

if [ $# -lt 1 ]; then
    echo "Usage: $0 <device>"
    exit 1
fi

DEVICE="$1"

SD_IMAGE="images/sd.img"

LINUX_SIZE=$(wc -c < images/image.bin)

echo -n $LINUX_SIZE > size.txt

echo "Write image size..."
dd if=size.txt bs=512 count=1 of="$SD_IMAGE"

echo "Write Linux image..."
dd conv=notrunc if=images/image.bin obs=512 seek=1 of="$SD_IMAGE"

echo "Flash SD card"
dd if="$SD_IMAGE" of="$DEVICE"

rm size.txt

echo "Done"
