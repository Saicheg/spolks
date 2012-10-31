#!/bin/sh
iptables-restore < $PWD/iptables-reject.rules
exit 0
