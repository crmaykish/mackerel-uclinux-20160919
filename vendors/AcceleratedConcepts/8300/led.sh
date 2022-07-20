#!/bin/sh
#
# take care of 8300 specific LED setting
#
# The most important thing here is to ensure that a LED is always on.
# if we have no LAN and no cell,  that means keep the ETH led flashing.
#
##############################################################
# allow script override
[ -x /etc/config/led.sh ] && exec /etc/config/led.sh "$@"
##############################################################

usage()
{
	[ "$1" ] && echo "$1"
	echo "usage: $0 <interface_lan|interface_modem> <setup|teardown|up|down>"
	exit 1
}

##############################################################

lan_led()
{
	case "$1" in
	setup)         ledcmd -f ONLINE ;;
	up)            ledcmd -o ONLINE ;;
	down|teardown) ledcmd -O ONLINE ;;
	esac
}

modem_led()
{
	case "$1" in
	setup)         ledcmd -f USB ;;
	up)            ledcmd -o USB ;;
	down|teardown) ledcmd -O USB ;;
	esac
}

signal_led()
{
	case "$1" in
		5) sig="-o RSS1 -o RSS2 -o RSS3 -o RSS4 -o RSS5" ;;
		4) sig="-o RSS1 -o RSS2 -o RSS3 -o RSS4 -O RSS5" ;;
		3) sig="-o RSS1 -o RSS2 -o RSS3 -O RSS4 -O RSS5" ;;
		2) sig="-o RSS1 -o RSS2 -O RSS3 -O RSS4 -O RSS5" ;;
		1) sig="-o RSS1 -O RSS2 -O RSS3 -O RSS4 -O RSS5" ;;
		*) sig="-O RSS1 -O RSS2 -O RSS3 -O RSS4 -O RSS5" ;;
	esac
	[ -x /usr/bin/ftdi-led ] && /usr/bin/ftdi-led $sig 2> /dev/null
}

rat_led()
{
	case "$1" in
	3g) rat="-o 3g -O 4g" ;;
	4g) rat="-O 3g -o 4g" ;;
	*)  rat="-O 3g -O 4g" ;;
	esac
	[ -x /usr/bin/ftdi-led ] && /usr/bin/ftdi-led $rat 2> /dev/null
}

sim_led()
{
	case "$1" in
	ok) mdm="-o mdm" ;;
	*)  mdm="-O mdm" ;;
	esac
	[ -x /usr/bin/ftdi-led ] && /usr/bin/ftdi-led $mdm 2> /dev/null
}

firmware_led()
{
	case "$1" in
	start) ledcmd -a -n ALL -f ALL ;;
	stop)  ledcmd -a -N ALL ;;
	esac
}

##############################################################

[ $# -ne 2 ] && usage "Wrong number of arguments"

CMD="$1"
shift

case "$CMD" in
*interface_lan*)   lan_led    "$@" ;;
link_lan)          lan_led    "$@" ;;
*interface_modem*) modem_led  "$@" ;;
signal*)           signal_led "$@" ;;
rat*)              rat_led    "$@" ;;
sim*)              sim_led    "$@" ;;
firmware*)         firmware_led "$@" ;;
*)                 exit 1 ;;
esac

exit 0
