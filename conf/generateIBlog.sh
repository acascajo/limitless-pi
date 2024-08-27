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

#while true
#do
	#while read IP; do
        	#echo "${IP}"
	#	scp -i ~/.ssh/id_rsa acascajo@${IP}:/tmp/log_${IP}* /tmp
	#done < ${host_file}

	rm /tmp/IB_log.csv
	touch /tmp/IB_log.csv
	cat /home/acascajo/.log_* >> /tmp/IB_log.csv
	#/tmp/log_* >> /tmp/IB_log.csv
	echo "Fin"
	#rm /tmp/log_*
#	sleep 7
#done

