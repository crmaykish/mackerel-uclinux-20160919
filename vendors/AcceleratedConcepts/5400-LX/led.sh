#!/bin/sh
#
# take care of 5400-LX specific LED setting
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
	echo "usage: $0 <interface_lan|interface_modem|signal|rat> <setup|teardown|up|down>"
	exit 1
}

##############################################################
#
# get our tools
#

source /usr/share/libubox/jshn.sh

##############################################################

lan_led()
{
	DEV=$(config get network.device.lan.device)
	LINK=$(ip link show "$DEV" 2> /dev/null | grep -c "state UP")

	LAN4=-1
	[ "$LINK" -ne 0 ] &&
		STATUS=$(ubus call network.interface.ipv4_interface_lan status 2> /dev/null)
	if [ "$?" = 0 ]; then
		LAN4=0
		eval $(jshn -r "$STATUS" 2> /dev/null)
		json_get_var pending "pending"
		json_get_var up "up"
		[ "${pending:-0}" -eq 1 ] && LAN4=1
		[ "${up:-0}" -eq 1 ] && LAN4=2
		[ "$LAN4" -gt 1 ] && [ "$(runt get network.netmon.child.ipv4_interface_lan.passed)" = n ] && LAN4=1
	fi

	LAN6=-1
	[ "$LINK" -ne 0 ] &&
		STATUS=$(ubus call network.interface.ipv6_interface_lan status 2> /dev/null)
	if [ "$?" = 0 ]; then
		LAN6=0
		eval $(jshn -r "$STATUS" 2> /dev/null)
		json_get_var pending "pending"
		json_get_var up "up"
		[ "${pending:-0}" -eq 1 ] && LAN6=1
		[ "${up:-0}" -eq 1 ] && LAN6=2
		[ "$LAN6" -gt 1 ] && [ "$(runt get network.netmon.child.ipv6_interface_lan.passed)" = n ] && LAN6=1
	fi

	# if neither interface exists force the LAN to match link
	[ "$LAN4" -eq -1 -a "$LAN6" -eq -1 ] && LAN4=$((LINK ? 2 : 0))

	if [ "$LAN4" -eq 2 -o "$LAN6" -eq 2 ]; then
		ledcmd -o ETH
	elif [ "$LAN4" -eq 1 -o "$LAN6" -eq 1 ]; then
		ledcmd -f ETH
	else
		ledcmd -O ETH
	fi
}

modem_led()
{
	case "$1" in
	up)
		[ "$(runt get network.netmon.child.interface_modem.passed)" = n ] &&
			ledcmd -f ONLINE || ledcmd -o ONLINE
	;;
	setup)         ledcmd -f ONLINE ;;
	down|teardown) ledcmd -O ONLINE ;;
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
	ledcmd $sig
}

rat_led()
{
	case "$1" in
	3g) ledcmd -O LAN3_RX -o LAN3_TX ;;
	4g) ledcmd -o LAN3_RX -O LAN3_TX ;;
	*)  ledcmd -O LAN3_RX -O LAN3_TX ;;
	esac
}

sim_led()
{
	:
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
