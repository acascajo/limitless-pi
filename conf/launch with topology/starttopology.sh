#!/bin/bash

port_client=""
time=""
port_flex=""
step=-1
master_ip=""
master_port=""
retrans_ip=""
retrans_port=""
file=""
while getopts "h?f:c:d:t:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage: ./starttopology -c <port_client> -t <db_time> -d <port_flexmpi> -f <file_topology.xml>"
        exit 0
        ;;
    c)  port_client=$OPTARG
        ;;
    t)  time=$OPTARG
        ;;
    d)  port_flex=$OPTARG
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
    
    #step=1 --> search retransmitter and nodes | step=0 --> search groups | step=2 --> search master
    if [ $step -eq 2 ]; then
	#echo "${line}"
        master_ip=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/ip/ {print $2}'`
        master_port=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/port/ {print $2}'`
	echo "Master IP: ${master_ip} Port: ${master_port}"
        ssh -n ${master_ip} "DISPLAY=:0 nohup $HOME/repo/Flex-Daemon/test_server/build/Test_server -p ${master_port} -c ${port_client} -t ${time} -f ${port_flex} < /dev/null > /dev/null 2> /dev/null &"
	step=-1
    elif [ $step -eq 1 ]; then
        if [[ $line == *"retransmitter"* ]]; then
            retrans_ip=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/ip/ {print $2}'`
            retrans_port=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/port/ {print $2}'`
            echo "Retransmitter IP: ${retrans_ip} Port: ${retrans_port}"
            ssh -n ${retrans_ip} "DISPLAY=:0 nohup $HOME/repo/Flex-Daemon/test_server/build/Test_server -p ${retrans_port} -s ${master_ip} -r ${master_port} < /dev/null > /dev/null 2> /dev/null &"
        elif [[ $line == *"node "* ]]; then
            node=`echo "${line}" | tr ' ' '\n' | awk -F'"' '/ip/ {print $2}'`
            echo "Node IP: ${node} [Parent: ${retrans_ip}]"
            ssh -n ${node} "DISPLAY=:0 nohup $HOME/repo/Flex-Daemon/build/DaeMon -s 1 -i 4000 -a ${retrans_ip} -p ${retrans_port} -b 0 < /dev/null > /dev/null 2> /dev/null &"
        fi
    elif [ $step -eq 0 ]; then
        retrans_ip=""
        retrans_port=""
    fi
done < "$file"
