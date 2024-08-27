#!/bin/bash

if [[ $# -eq 0 ]] ; then
    echo 'Usage: ./watchdog IP port'
    exit 0
fi

while true; do
	result=(`ps -aux |grep DaeMon | wc -l`)
	#echo "Resultado $result"
	if [ "$result" = "1" ]; then
		/home/alberto/Escritorio/merge/Flex-Daemon/build/DaeMon -s 1 -i 30000 -a $1 -p $2 -b 0 -n 0 < /dev/null > /dev/null 2> /dev/null &
	fi
	sleep 30
done
echo "Something went wrong"

