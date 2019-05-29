#!/bin/bash
## Change where files are located
sin_deamon_name="sind"
sin_deamon = "/root/SIN-core/src/sind"
sin_cli="/root/SIN-core/src/sin-cli"
##

function start_node() {
	sleep 5
	echo "restart sin deamon" >> ~/.sin/sin_control.log
	eval $sin_deamon &
}

function stop_start_node() {
	echo "kill process by name $sin_deamon" >> ~/.sin/sin_control.log
	pgrep -f $sin_deamon_name | awk '{print "kill -9 " $1}' | sh >> ~/.sin/sin_control.log
	sleep 5
	echo "restart sin deamon" >> ~/.sin/sin_control.log
	eval $sin_deamon &
}

DATE_WITH_TIME=`date "+%Y%m%d-%H:%M:%S"`
timeout --preserve-status 10 $sin_cli getblockcount
CHECK_SIN=$?
echo $CHECK_SIN >> ~/.sin/sin_control.log
if [ "$CHECK_SIN" -eq "0" ]; then
	echo "$DATE_WITH_TIME sin deamon is active" >> ~/.sin/sin_control.log
fi

#node is stopped
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
	echo "Command not found. Please change the path of sin_deamon and sin_cli." >> ~/.sin/sin_control.log
fi

# node is frozen
if [ "$CHECK_SIN" -eq "143" ]; then
	echo "$DATE_WITH_TIME sin deamon will be restarted...." >> ~/.sin/sin_control.log
	stop_start_node
fi
