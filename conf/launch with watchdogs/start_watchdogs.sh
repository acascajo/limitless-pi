#!/bin/bash



host_file=""
master_address=""
port_master=""
port_client=""
time=""
port_flex=""
mem_hot=""
cache_hot=""
net_hot=""
while getopts "h?f:m:p:c:d:t:q:w:e:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    m)  master_address=$OPTARG
        ;;
    p)	port_master=$OPTARG
	;;	
    f)  host_file=$OPTARG
        ;;
    c)  port_client=$OPTARG
	;;
    t)  time=$OPTARG
	;;
    d)  port_flex=$OPTARG
        ;;
    q)  mem_hot=$OPTARG
	;;
    w)  cache_hot=$OPTARG
	;;
    e)  net_hot=$OPTARG
	;;
    esac
done



if [ ! -f "$host_file" ]; then
	echo "It is not a valid file"
	exit
fi


ssh ${master_address} "DISPLAY=:0 nohup /home/alberto/Escritorio/merge/Flex-Daemon/conf/watchdog_server.sh ${port_master} ${port_client} ${time} ${port_flex} ${mem_hot} ${cache_hot} ${net_hot} < /dev/null > /dev/null 2> /dev/null &"


while read IP; do
	echo "${IP}"
	ssh -n ${IP} "DISPLAY=:0 nohup /home/alberto/Escritorio/merge/Flex-Daemon/conf/watchdog_daemon.sh ${master_address} ${port_master} < /dev/null > /dev/null 2> /dev/null &"
done < ${host_file}
