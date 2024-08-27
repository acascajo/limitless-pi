#!/bin/bash

#$1 port_master
#$2 port_client
#$3 time to heartbeat
#$4 port_flex
#$5-6-7 Hot-spot thresholds (mem, cache, net)


if [[ $# -eq 0 ]] ; then
    echo 'Usage: ./watchdog port_master port client time_hb port_flex mem_hs cache_hs net_hs'
    exit 0
fi

while true; do
	result=(`ps -aux |grep Test_server | wc -l`)
	#echo "Resultado $result"
	if [ "$result" = "1" ]; then
		/home/alberto/Escritorio/merge/Flex-Daemon/test_server/build/Test_server -p $1 -c $2 -t $3 -f $4 -n 0 -q $5 -w $6 -e $7 < /dev/null > /dev/null 2> /dev/null &
	fi
	sleep 30
done
echo "Something went wrong"

