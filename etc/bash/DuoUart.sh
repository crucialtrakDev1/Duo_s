#!/bin/bash

ps -elf | grep BACS | grep DuoUart | awk '{print $4}' | while read line; do sudo kill -9 $line; done


while true;
do
redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
        echo "already started redis-server"

	check="`pgrep BACSDuoUart | wc -l`"
	if [ "$check" -eq "0" ] ; then
	echo "start BACSDuoUart"
	cd host
	sudo ./BACSDuoUart
	fi
	
fi
sleep 5
done
