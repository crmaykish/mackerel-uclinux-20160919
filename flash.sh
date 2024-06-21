#!/bin/sh

PORT=/dev/ttyUSB0
IMAGE_FILE="images/mackerel.bin"
ROMFS_FILE="images/romfs.img"

echo "Copying ROMFS, size: $(stat --printf="%s" $ROMFS_FILE)"
echo "load 300000" > $PORT
sleep 1
ctt -p $PORT -f "$ROMFS_FILE"

sleep 1

echo "Copying Linux image, size: $(stat --printf="%s" $IMAGE_FILE)"
echo "load" > $PORT
sleep 1
ctt -p $PORT -f "$IMAGE_FILE"

sleep 1
echo "Booting Linux..."
echo "run" > $PORT

echo "Done"
