#!/bin/bash

while true;
do

redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
        echo "Ready redis-server"

	check="`ps -elf | grep nodejs | grep bacsapi.js | awk '{print $4}'`"
	
	if [ $check=="" ] ; then
	echo "start BACSApi"
	cd /home/bacs/host
	sudo nodejs ./bacsapi.js
	fi
fi
sleep 1
done
