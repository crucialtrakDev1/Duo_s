#!/bin/bash

ps -elf | grep BACS | grep DuoRfid | awk '{print $4}' | while read line; do sudo kill -9 $line; done


while true;
do
sleep 5
redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
        echo "already started redis-server"

	check="`pgrep BACSDuoRfid | wc -l`"
	if [ "$check" -eq "0" ] ; then
	echo "start BACSDuoRfid"
	cd host
	sudo ./BACSDuoRfid
	fi
	
fi
sleep 5
done
