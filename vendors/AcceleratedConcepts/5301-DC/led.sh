#!/bin/sh
#
# take care of 5301-DC specific LED setting
#
##############################################################
# allow script override
[ -x /etc/config/led.sh ] && exec /etc/config/led.sh "$@"
##############################################################
#exec 2>> /tmp/led.log
#set -x

usage()
{
	[ "$1" ] && echo "$1"
	echo "usage: $0 <interface_wan|interface_dialin> <setup|teardown|up|down>"
	exit 1
}

##############################################################
#
# get our tools
#

source /usr/share/libubox/jshn.sh

##############################################################
#
# do ethernet,  we always have ethernet even if not plugged in
# WAN 0=not there, 1=starting, 2=up
#

WAN4=0
STATUS=$(ubus call network.interface.ipv4_interface_wan status 2> /dev/null)
if [ "$?" = 0 ]; then
	eval $(jshn -r "$STATUS" 2> /dev/null)
	json_get_var pending "pending"
	json_get_var up "up"
	[ "${pending:-0}" -eq 1 ] && WAN4=1
	[ "${up:-0}" -eq 1 ] && WAN4=2
fi

WAN6=0
STATUS=$(ubus call network.interface.ipv6_interface_wan status 2> /dev/null)
if [ "$?" = 0 ]; then
	eval $(jshn -r "$STATUS" 2> /dev/null)
	json_get_var pending "pending"
	json_get_var up "up"
	[ "${pending:-0}" -eq 1 ] && WAN6=1
	[ "${up:-0}" -eq 1 ] && WAN6=2
fi

if [ "$WAN4" -eq 2 -o "$WAN6" -eq 2 ]; then
	WAN=2
elif [ "$WAN4" -eq 1 -o "$WAN6" -eq 1 ]; then
	WAN=1
else
	WAN=0
fi

#
# The LED table of things based on all possible values above
#
LED0="-f COM -O ONLINE -f ETH" # flashing yellow
LED1="-f COM -O ONLINE -f ETH" # flashing yellow
LED2="-O COM -O ONLINE -o ETH" # solid green

##############################################################

wan_led()
{
	eval ledcmd \$LED$WAN
}

dialin_led()
{
	eval ledcmd \$LED$WAN
	case "$1" in
	setup)         ledcmd -a -n COM -n ETH -n ONLINE -f COM -O ETH -f ONLINE ;;
	up)            ledcmd -a -o COM -O ETH -o ONLINE -n COM -n ETH -n ONLINE ;;
	down|teardown) ledcmd -a -N COM -N ETH -N ONLINE ;;
	esac
}

firmware_led()
{
	case "$1" in
	start) ledcmd -a -n COM -n ETH -n ONLINE -o COM -f ETH -O ONLINE ;;
	stop)  ledcmd -a -N COM -N ETH -N ONLINE ;;
	esac
}

##############################################################

[ $# -ne 2 ] && usage "Wrong number of arguments"

CMD="$1"
shift

case "$CMD" in
*interface_wan*)    wan_led  "$@" ;;
*interface_dialin*) dialin_led "$@" ;;
firmware*)          firmware_led "$@" ;;
*)                  exit 1 ;;
esac

exit 0
