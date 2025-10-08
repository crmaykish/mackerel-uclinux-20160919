#!/bin/sh

sudo mount /dev/sda1 /mnt/sd
sudo cp images/image.bin /mnt/sd/IMAGE.BIN
sync
sudo umount /mnt/sd
echo "Done."
