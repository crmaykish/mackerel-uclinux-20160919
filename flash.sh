#!/bin/sh

PORT=/dev/ttyACM0
IMAGE_FILE="images/image.bin"

ls -lah "$IMAGE_FILE"

echo "Copying Linux image..."
echo "load" > $PORT
sleep 1
ctt -p $PORT -f "$IMAGE_FILE"

sleep 1
echo "Booting Linux..."
echo "run" > $PORT

echo "Done"
