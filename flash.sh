#!/bin/sh

PORT=/dev/ttyUSB0
IMAGE_FILE="images/image.bin"
ROMFS_FILE="images/romfs.img"

ls -lah "$ROMFS_FILE"

echo "Copying ROMfs..."
echo "load 300000" > $PORT
sleep 1
ctt -p $PORT -f "$ROMFS_FILE"

sleep 1

ls -lah "$IMAGE_FILE"

echo "Copying Linux image..."
echo "load" > $PORT
sleep 1
ctt -p $PORT -f "$IMAGE_FILE"

sleep 1
echo "Booting Linux..."
echo "run" > $PORT

echo "Done"
