#!/bin/sh
#
# take care of X86 specific LED setting
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
	echo "usage: $0 <not-implemented> <setup|teardown|up|down>"
	exit 1
}

##############################################################

[ $# -ne 2 ] && usage "Wrong number of arguments"

CMD="$1"
shift

case "$CMD" in
*interface_dialin*) : not implemented ;;
*interface_wan*)    : not implemented ;;
*interface_modem*)  : not implemented ;;
*interface_dialin*) : not implemented ;;
signal*)            : not implemented ;;
rat*)               : not implemented ;;
sim*)               : not implemented ;;
firmware*)          : not implemented ;;
*)                  usage "bad led target - $CMD" ;;
esac

exit 0
