#!/bin/bash

step=-1
file=""
while getopts "h?f:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage: ./stoptopology -f <file_topology.xml>"
        exit 0
        ;;
    f)  file=$OPTARG
        ;;
    esac
done

while IFS='' read -r line || [[ -n "$line" ]]; do
    #echo "Text read from file: $line"
    if [[ $line == *"<node-group>"* ]]; then
	step=1
    elif [[ $line == *"</node-group>"* ]]; then 
        step=0
    elif [[ $line == *"node-master"* ]]; then
        step=2
    fi
    
    if [ $step -eq 2 ]; then
	#echo "${line}"
        master_ip=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/ip/ {print $2}'`
	echo "Master IP: ${master_ip} Port: ${master_port}"
        ssh -n ${master_ip} "pkill -9 Test_server"
	step=-1
    elif [ $step -eq 1 ]; then
        if [[ $line == *"retransmitter"* ]]; then
            retrans_ip=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/ip/ {print $2}'`
            echo "Retransmitter IP: ${retrans_ip} Port: ${retrans_port}"
            ssh -n ${retrans_ip} "pkill -9 Test_server"
        elif [[ $line == *"node "* ]]; then
            node=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/ip/ {print $2}'`
            echo "Node IP: ${node} [Parent: ${retrans_ip}]"
            ssh -n ${node} "pkill -9 DaeMon"
        fi
    fi
done < "$file"
