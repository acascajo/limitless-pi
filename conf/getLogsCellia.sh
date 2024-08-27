#!/bin/bash

host_file=""
while getopts "h?f:" opt; do
    case "$opt" in
    h|\?)
        echo "./getData -f <hosts>"
        exit 0
        ;;
    f)  host_file=$OPTARG
        ;;
    esac
done

mkdir logs
while read IP; do
        echo "${IP}"
	scp -i ~/.ssh/id_rsa acascajo@${IP}:/tmp/log_${IP}* ./logs
	ssh -n ${IP} "rm /tmp/log_nodo*"
done < ${host_file}

