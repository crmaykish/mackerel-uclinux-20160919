#!/bin/sh

PORT=/dev/ttyACM0

echo "Copying filesystem image..."
echo "load 300000" > $PORT
sleep 1
ctt -p $PORT -f images/romfs.img

sleep 1

echo "Copying Linux kernel..."
echo "load 8000" > $PORT
sleep 1
ctt -p $PORT -f images/image.bin

sleep 1
echo "Booting Linux..."
echo "run" > $PORT

echo "Done"
