#!/bin/bash

ps -elf | grep BACS | grep DuoLcd | awk '{print $4}' | while read line; do sudo kill -9 $line; done


while true;
do
redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
        echo "already started redis-server"

	check="`pgrep BACSDuoLcd | wc -l`"
	if [ "$check" -eq "0" ] ; then
	echo "start BACSDuoLcd"
	cd host
	sudo ./BACSDuoLcd
	fi
	
fi
sleep 5
done
