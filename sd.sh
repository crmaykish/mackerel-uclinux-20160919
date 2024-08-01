#!/bin/sh

DEVICE=/dev/sdb

SD_IMAGE="images/sd.img"

echo "Write kernel"
dd if=images/image.bin bs=512 count=2048 of="$SD_IMAGE"

echo "Write romfs"
dd conv=notrunc if=images/romfs.img obs=512 seek=2048 of="$SD_IMAGE"

echo "Flash SD card"
dd if="$SD_IMAGE" of="$DEVICE"

echo "Done"
