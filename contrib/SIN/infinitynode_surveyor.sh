#!/bin/bash
# Copyright (c) 2019 The SIN Core developers
# Auth: xtdevcoin
#
# this script control the status of node
# 1. node is active
# 2. infinitynode is ENABLED
# 3. infinitynode is not ENABLED
# 4. node is stopped by supplier - maintenance
# 5. node is frozen - dead lock
#
# Add in crontab when YOUR NODE HAS STATUS ENABLED:
# */5 * * * * /full_path_to/infinitynode_surveyor.sh
#
# change path of "sin_deamon" and "sin_cli"
#

sin_deamon_name="sind"

## PLEASE CHANGE THIS
sin_deamon="/path/SIN-core/src/sind"
sin_cli="/path/SIN-core/src/sin-cli"
##

DATE_WITH_TIME=`date "+%Y%m%d-%H:%M:%S"`

function start_node() {
	sleep 5
	echo "$DATE_WITH_TIME : Start sin deamon $sin_deamon" >> ~/.sin/sin_control.log
	$sin_deamon &
}

function stop_start_node() {
	echo "$DATE_WITH_TIME : kill process by name $sin_deamon_name" >> ~/.sin/sin_control.log
	pgrep -f $sin_deamon_name | awk '{print "kill -9 " $1}' | sh >> ~/.sin/sin_control.log
	sleep 15
	echo "$DATE_WITH_TIME : Restart sin deamon $sin_deamon" >> ~/.sin/sin_control.log
	$sin_deamon &
}

timeout --preserve-status 10 $sin_cli getblockcount
CHECK_SIN=$?
echo "$DATE_WITH_TIME : check status of sind: $CHECK_SIN" >> ~/.sin/sin_control.log

#node is active
if [ "$CHECK_SIN" -eq "0" ]; then
	echo "$DATE_WITH_TIME : sin deamon is active" >> ~/.sin/sin_control.log
	SINSTATUS=`$sin_cli masternode status | grep "successfully" | wc -l`

	#infinitynode is ENABLED
	if [ "$SINSTATUS" -eq "1" ]; then
		echo "$DATE_WITH_TIME : infinitynode is active" >> ~/.sin/sin_control.log
	#infinitynode is not ENABLED
	else
		echo "$DATE_WITH_TIME : infinitynode is not ENABLED" >> ~/.sin/sin_control.log
		echo "$DATE_WITH_TIME : Restarting..." >> ~/.sin/sin_control.log
		stop_start_node
	fi
fi

#node is stopped by supplier - maintenance
if [ "$CHECK_SIN" -eq "1" ]; then
	#find sind
	SIND=`ps -e | grep $sin_deamon_name | wc -l`
	if [ "$SIND" -eq "0" ]; then
		start_node
	else
		stop_start_node
	fi
fi

#command not found
if [ "$CHECK_SIN" -eq "127" ]; then
	echo "$DATE_WITH_TIME : Command not found. Please change the path of sin_deamon and sin_cli." >> ~/.sin/sin_control.log
fi

#node is frozen
if [ "$CHECK_SIN" -eq "143" ]; then
	echo "$DATE_WITH_TIME : sin deamon will be restarted...." >> ~/.sin/sin_control.log
	stop_start_node
fi
