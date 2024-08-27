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
while getopts "h?f:m:p:" opt; do
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
    esac
done



if [ ! -f "$host_file" ]; then
	echo "It is not a valid file"
	exit
fi


while read IP; do
	echo "${IP}"
	ssh -n ${IP} "DISPLAY=:0 nohup /home/acascajo/repo/Limitless_bak/build/DaeMon -s 1 -i 10000 -a ${master_address} -p ${port_master} -b 0 -n 0 < /dev/null > /dev/null 2> /dev/null &"
done < ${host_file}
