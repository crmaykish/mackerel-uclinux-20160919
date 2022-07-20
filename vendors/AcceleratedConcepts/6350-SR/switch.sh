#
# switch.sh  -- initialize networking hardware
#

#
# Use port based VLANs to partition the switch ports.
# Physical ports 0-3 are hooked up to digital port 5 (eth0)
# Physical port 4 is hooked up to digital port 6 (eth1)
#
swtest -i eth0 -p 16 -a 6 -w 46
swtest -i eth0 -p 17 -a 6 -w 45
swtest -i eth0 -p 18 -a 6 -w 43
swtest -i eth0 -p 19 -a 6 -w 39
swtest -i eth0 -p 20 -a 6 -w 64
swtest -i eth0 -p 21 -a 6 -w 15
swtest -i eth0 -p 22 -a 6 -w 16

#
# Set up LED actions
#
swtest -i eth0 -p 16 -a 22 -w 0x8088
swtest -i eth0 -p 17 -a 22 -w 0x8088
swtest -i eth0 -p 18 -a 22 -w 0x8088
swtest -i eth0 -p 19 -a 22 -w 0x8088
swtest -i eth0 -p 20 -a 22 -w 0x8088

exit 0
