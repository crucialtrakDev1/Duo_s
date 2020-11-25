#!/bin/bash

ps -elf | grep BACS | grep DuoTof | awk '{print $4}' | while read line; do sudo kill -9 $line; done


while true;
do
redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
        echo "already started redis-server"

	check="`pgrep BACSDuoTof | wc -l`"
	if [ "$check" -eq "0" ] ; then
	#echo "start BACSDuoTof"
	#cd host
	#sudo ./BACSDuoTof
	fi
	
fi
sleep 5
done
