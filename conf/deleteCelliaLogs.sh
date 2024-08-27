#!/bin/bash



host_file=""
master_address=""
while getopts "h?f:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
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
	ssh -n ${IP} "rm /tmp/log_nodo*"
done < ${host_file}

