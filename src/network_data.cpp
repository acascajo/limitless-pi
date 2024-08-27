#ifndef NETWORK_DATA
#define NETWORK_DATA

#include <arpa/inet.h>
#include "Packed_sample.hpp"
#include "network_data.hpp"
#include "server_data.hpp"
#include "servidor_monitor.hpp"
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <iostream>
#include "common.hpp"
#include <fstream>
#include <mutex>
#include <arpa/inet.h>
#include <string.h>
#include <library.h>
#include <netdb.h>
#include <math.h>
#include "connector.h"

using namespace std;


std::mutex logging_lock;

/*David wants this to show inormation about some tests*/
int displaySample=0;
std::string displayedMes="";
int dbSaveCounter = 10;
int es_count = 0;
char es_line[20000];
//char es_line2[20000];
int es_wpointer = 0;


int convert_char_to_short_int(unsigned char * buffer, unsigned short int *number){

	*number = ((int)buffer[0]<<8) | (int) buffer[1];

        return 0;
}

int calculate_conf_packet(Hw_conf hw_conf){

	//return	CONF_PACKET_SIZE + (hw_conf.n_gpu * 3);
	return 0;

}

int convert_int_to_char(unsigned char * buffer, unsigned int number){
  	int size = sizeof(unsigned int);

	for(int i= 1; i <= size; i++ ){
		buffer[i - 1] = (unsigned char)(number >> (8 * (size - i))) & (0xff);
	}

	return size;
}

int convert_char_to_int(unsigned char * buffer){

	int size = sizeof(unsigned int);
	unsigned int number = 0;
	for(int i= 1; i <= size; i++){
		unsigned int tmp = (unsigned int) buffer[i-1];
		number = (tmp << ((size-i)*8)) | number ;
	}

	return number;
}

int convert_time_to_char(unsigned char * buffer, time_t time){
	int size = sizeof(time_t);

	for(int i= 1; i <= size; i++ ){
		buffer[i - 1] = (unsigned char)(time >> (8 * (size - i))) & (0xff);
	}

	return size;
}

time_t obtain_time_from_packet(unsigned char * buffer){
	int size = sizeof(time_t);
	time_t time_sample = 0;
	for(int i= 1; i <= size; i++){
		time_t tmp = (time_t) buffer[i-1];
		time_sample = (tmp << ((size-i)*8)) | time_sample ;
	}

	return time_sample;
}

/* Function to print the raw packed sample on cout */
int print_raw_packet_sample(unsigned char * buffer, int size, int n_io_devices, int n_net_devices, int n_core, int n_gpu, std::string ip) {

//	int counter = 4;
	int counter = 0;
	int i = 0;
	int error = 0;
	/*Obtain memory usage*/
	int mem_usage_perc = (int) buffer[counter];
	vector<int> io_devices_w_perc(n_io_devices);
	vector<int> io_devices_io_perc(n_io_devices);
	vector<int> net_devices_speed(n_net_devices);
	vector<int> net_devices_usage_perc(n_net_devices);
	vector<int> temp_core(n_core);
	vector<int> temp_core_perc(n_core);
	vector<int> pw_cpu(n_core);
	vector<int> gpu_memUsage(n_gpu);
	vector<int> gpu_Usage(n_gpu);
	vector<int> gpu_temp(n_gpu);
	vector<int> gpu_pw(n_gpu);

	counter += SIZE_PACKED_PERCENTAGE;

	/*Obtain CPU_idle*/
	int CPUidle_perc = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;
	//counter +=  SIZE_PACKED_PERCENTAGE;

	/*For each cpu, obtain power usage in Joules*/
	for (i = 0; i < n_core; ++i) {
		pw_cpu[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
	}

	/*For each device, obtain w(%) and TIO(%)*/
	for (i = 0; i < n_io_devices; ++i) {
		io_devices_w_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		io_devices_io_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
	}
	/*For each network device, obtain speed and network usage*/
	for (i = 0; i < n_net_devices; ++i) {
		net_devices_speed[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		net_devices_usage_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;

	}
	/*For each coretemp obtain temp(%)*/
	for (i = 0; i < n_core; ++i) {
		temp_core[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		temp_core_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;

	}

	/*For each gpu obtain gpu features*/
	for (i = 0; i < n_gpu; ++i) {
		//Saltamos el cudacomp	
//		counter +=  SIZE_PACKED_PERCENTAGE;
		gpu_memUsage[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		gpu_Usage[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		gpu_temp[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		gpu_pw[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;

		//temp_core_perc[i] = (int) buffer[counter];
		//counter +=  SIZE_PACKED_PERCENTAGE;

	}

	int cache_ratio = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;

	int cpu_stalled = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;

	/*int siblings = (int) buffer[counter];
	counter++;
	vector<int> core_load;
	for(int i = 0; i < siblings; i++){
	    int load = (int) buffer[counter];
	    counter++;
        core_load.push_back(load);
	}*/


	if (displaySample == 1) {
		/*ofstream myfile;
		myfile.open("/tmp/log_monitor/log_v2.xt", ios::out | ios::app);
		myfile << displayedMes << endl;
		myfile << "Mem usage: " << mem_usage_perc << "%" << endl;
		myfile << "CPU Busy : " << CPUidle_perc << "%" << endl;
		//myfile << "Energy usage: " << pw_cpu << "Joules" << endl;
		//myfile << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
		myfile << "Net devices usage" << net_devices_usage_perc << "%" << endl;
		//myfile << "Temp CPU: " << temp_core << "Cº" << endl;
		//myfile << "Temp percentage: " << temp_core_perc << "%" << endl;
		myfile << "Cache: " << cache_ratio << "%" << endl;
		myfile << "Cpu stalled: " << cpu_stalled << "%" << endl << endl << endl;
		myfile.close();*/

		/*cout << displayedMes << endl;
        cout << "Mem usage: " << mem_usage_perc << "%" << endl;
        cout << "CPU Busy : " << CPUidle_perc << "%" << endl;
        cout << "Energy usage: " << pw_cpu << "Joules" << endl;
        cout << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
        cout << "Net devices usage" << net_devices_usage_perc << "%" << endl;
        cout << "Temp CPU: " << temp_core << "Cº" << endl;
        cout << "Temp percentage: " << temp_core_perc << "%" << endl;
        cout << "Cache:" << cache_ratio << "%" << endl;*/

		displaySample = 0;
		displayedMes = "";
	}

	/*cout << "Mem usage: " << mem_usage_perc << "%" << endl;
    cout << "CPU Busy : " << CPUidle_perc << "%" << endl;
    cout << "Energy usage: " << pw_cpu << "Joules" << endl;
    cout << "IO devices W : " << io_devices_w_perc << "%" << endl;
    cout << "IO devices IO : " << io_devices_io_perc << "%" << endl;
    cout << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
    cout << "Net devices usage" << net_devices_usage_perc << "%" << endl;
    cout << "Temp CPU: " << temp_core << "Cº" << endl;
    cout << "Temp percentage: " << temp_core_perc << "%" << endl;
    cout << "GPU Mem usage: " << gpu_memUsage << "%" << endl;
    cout << "GPU Usage:" << gpu_Usage << "%" << endl;
    cout << "GPU temperature:" << gpu_temp << " Cº" << endl;
    cout << "GPU power usage:" << gpu_pw << " Watts" << endl;
    cout << "Cache:" << cache_ratio << endl;
    for (int i = 0; i < siblings; i++){
        cout << "vCore " << i << ": " << core_load[i] << "%" << endl;
    }*/

	/*auto time = std::chrono::system_clock::now();
	std::time_t now = std::chrono::system_clock::to_time_t(time);
    ofstream myfile;
    myfile.open("/tmp/log_daemon/log_energy.txt", ios::out | ios::app);
    myfile << "*********************" << endl;
    myfile << "IP: " << ip << endl;
    myfile << "TIME: " << std::ctime(&now) << endl;
    myfile << "Mem usage: " << mem_usage_perc << "%" << endl;
    myfile << "CPU Busy : " << CPUidle_perc << "%" << endl;
    myfile << "Energy usage: " << pw_cpu << "Joules" << endl;
    //myfile << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
    //myfile << "Net devices usage" << net_devices_usage_perc << "%" << endl;
    myfile << "Temp CPU: " << temp_core << "Cº" << endl;
    myfile << "Temp percentage: " << temp_core_perc << "%" << endl;
    myfile.close();
	*/

	/*if (dbSaveCounter == 10){
        backupDb("/tmp/db_bak");
        dbSaveCounter = 0;
	}
	dbSaveCounter++;*/

	return error;

}

/**

	Create the configuration packet
	@param[] hw_conf:
	@param[] conf_packed:

*/
int create_conf_packet(Hw_conf hw_conf, char * conf_packed){

	unsigned char mode = 0; //This variable indicate if mode is configuration (mode = 0), or information packet (mode != 0)

	calculate_conf_packet(hw_conf);

	int cpp = 0; //conf packet pointer
	conf_packed[0] = mode;
	conf_packed[1] = (unsigned char) hw_conf.n_cpu;
	conf_packed[2] = (unsigned char) hw_conf.n_cores;
	convert_short_int_to_char(&conf_packed[3],(unsigned short int) hw_conf.mem_total);
	conf_packed[5] = (unsigned char) hw_conf.n_devices_io;
	conf_packed[6] = (unsigned char) hw_conf.n_interfaces;

	/* Number of GPUs */
	conf_packed[7] = (unsigned char)hw_conf.n_gpu;
//	cout << "------------------------------------------------Creando el paquete de configuracion: el numero de gpus es: "<< hw_conf.n_gpu  << endl;
	cpp = 7;
	cpp ++;
	/*for(int i = 0; i < hw_conf.n_gpu; i++){
		//Cuda compatible
		#ifdef DEBUG_DAEMON
			//cout << "se va a guardar en la posicion: " << cpp << " el valor de cuda comp: " << hw_conf.gpus[i].cudacomp << endl;
		#endif
		
		conf_packed[cpp] = (unsigned char) hw_conf.gpus[i].cudacomp;
		cpp ++;
		//Total memory of the GPU
		conf_packed[cpp] = (unsigned char) hw_conf.gpus[i].memTotal;
		cpp ++;
		//GPU cpabilities
		conf_packed[cpp] = (unsigned char) hw_conf.gpus[i].capability;
		cpp ++;
	}*/
	conf_packed[cpp] = hw_conf.hostname.length();
	cpp++;	

	memcpy(&conf_packed[cpp],hw_conf.hostname.c_str(),hw_conf.hostname.length());
	cpp += hw_conf.hostname.length();

	/* Obtener modo bitmap */
	//cpp++;
	//conf_packed[cpp] = (unsigned char)hw_conf.modo_bitmap;
	//	printf("------------------------Modo del bitmap es: %d --------------\n",conf_packed[cpp]);
	//cout << "---------------------Modo del bitmap es: " << conf_packed[cpp] << endl;
	//cpp++;

	#ifdef DEBUG_DAEMON
		for(int i = 0; i < cpp; i++){
			printf("-------- posicion %d del conf_packet contiene %d \n",i,conf_packed[i]);
		}
	#endif
	
	

	return cpp;

}

/**
	Function to obtain the configuration information from a received raw packet.
	@param[in] conf_packed: char array that is a raw packet in which configuration information has been sended
	@param[in,out] hw_conf: hw_conf object build from conf_packed packet
	@param[in]: client_addr:

*/
int obtain_conf_from_packet(unsigned char * conf_packed, Hw_conf* hw_conf, const std::string client_addr){
	int error = 0;
	int counter = 0;
	unsigned short int tmp =0;
	int hostname_size = 0;
	//FIXME: Check for problems of memory scope
	//cout << "obtain conf from packet ///////////////////////////////////////////////////" << endl;
	hw_conf->ip_addr_s = client_addr;

	hw_conf->n_cpu = (int)conf_packed[counter];
  	counter++;

	hw_conf->n_cores = (int)conf_packed[counter];
	counter++;

	convert_char_to_short_int(&conf_packed[counter],&tmp);
	hw_conf->mem_total = (unsigned short int) tmp;
	counter+=sizeof(short int);

	hw_conf->n_devices_io=(int)conf_packed[counter];
	counter++;

	hw_conf->n_interfaces= (int)conf_packed[counter];
	counter ++;

	hw_conf->n_core_temps= hw_conf->n_cpu;

	hw_conf->n_gpu = (int)conf_packed[counter];
	counter++;


	/*for(int i = 0; i < hw_conf->n_gpu; i++){
		Gpu_dev gpudev;
		gpudev.cudacomp = (int)conf_packed[counter];
		counter++;
		gpudev.memTotal = (int)conf_packed[counter];
		counter++;
//		printf("******************************************************************************************************se esta guardando la capability: %d \n",conf_packed[counter]);
		gpudev.capability = (unsigned char)conf_packed[counter];
		counter++;
		hw_conf->gpus.push_back(gpudev);
	}*/

	hostname_size = conf_packed[counter];

	//cout << "****************************************** size hostname: "<< hostname_size << endl;

	counter++;
	for(int i=0; i < hostname_size; i++){
		stringstream ss;
		ss << (char) conf_packed[counter];
//		char aux = (char) conf_packed[counter];
		counter++;
		hw_conf->hostname.append(ss.str());
//		ss << conf_packed[counter];
//		hw_conf->hostname << (char)conf_packed[counter];
//		ss >> hostname;
//		counter++;
//		hw_conf->hostname.append(ss);
	}
	
	/* Obtener modo bitmap */
	//hw_conf->modo_bitmap = conf_packed[counter];
	//counter++;
	//printf("En el servidor se obtiene el modo de bitmap de: %d\n", hw_conf->modo_bitmap);

	/*std::string new_ip;
	for (int i = 0; i < 4; i++){
		new_ip = new_ip + std::to_string((int)conf_packed[counter+i]);
		if (i < 3)
            new_ip = new_ip + ".";
	}
	hw_conf->ip_addr_s = new_ip;*/

	#ifdef DEBUG_DAEMON
		/*for(int i = 0; i < (counter+1); i++ ){
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Posicion del paquete recibido %d contiene %d \n",i,conf_packed[i]);
		}*/
	#endif
	
	return error;
}

/**
 * Extract all data from a packet sample and save the information into the database
 * @param buffer All data as stream
 * @param n_io_devices Number of IO devices.
 * @param n_net_devices Number of net interfaces.
 * @param n_core Number of processors.
 * @param n_gpu Number of GPUs
 * @param ip_add Node IP address.
 * @return 0 if everything is OK, negative value in other way.
 */
int import_mon_packet_to_database(unsigned char * buffer, int n_io_devices, int n_net_devices, int n_core, int n_gpu, std::string ip_add, time_t time_sample) {

	int error = 0;

	int counter = 0;
	int i = 0;

	/*Obtain memory usage*/
	int mem_usage_perc = (int) buffer[counter];
	vector<int> io_devices_w_perc(n_io_devices);
	vector<int> io_devices_io_perc(n_io_devices);
	vector<int> net_devices_speed(n_net_devices);
	vector<int> net_devices_usage_perc(n_net_devices);
	vector<int> temp_core(n_core);
	vector<int> temp_core_perc(n_core);
	vector<int> pw_cpu(n_core);
	vector<int> gpu_memUsage(n_gpu);
	vector<int> gpu_Usage(n_gpu);
	vector<int> gpu_temp(n_gpu);
	vector<int> gpu_pw(n_gpu);

	counter += SIZE_PACKED_PERCENTAGE;

	/*Obtain CPU_idle*/
	int  CPUidle_perc = (int) buffer[counter];
	counter +=  SIZE_PACKED_PERCENTAGE;
	//counter +=  SIZE_PACKED_PERCENTAGE;

	/*For each cpu, obtain power usage in Joules*/
	for (i = 0; i< n_core; ++i){
		pw_cpu[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
	}

	/*For each device, obtain w(%) and TIO(%)*/
	for (i = 0; i< n_io_devices; ++i){
		io_devices_w_perc[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		io_devices_io_perc[i]=(int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
	}
	/*For each network device, obtain speed and network usage*/
	int net_g = 0;
	for (i = 0; i< n_net_devices; ++i){
		net_devices_speed[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		net_devices_usage_perc[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
        net_g += net_devices_usage_perc[i];//net_devices_speed[i];
	}
	if (n_net_devices != 0)
	    net_g = net_g / n_net_devices;

	/*For each coretemp obtain temp(%)*/
	int temp_g = 0;
	for (i = 0; i< n_core; ++i){
		temp_core[i] =(int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		temp_core_perc[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
        /*accumulate temp and divide --> aprox*/
        temp_g += temp_core[i];
	}
	temp_g = temp_g/n_core;

	/*For each gpu obtain gpu features*/
	for (i = 0; i< n_gpu; ++i){
		//Saltamos el cudacomp
//		counter +=  SIZE_PACKED_PERCENTAGE;
		gpu_memUsage[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		gpu_Usage[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		gpu_temp[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		gpu_pw[i] = (int) buffer[counter];
		counter +=  SIZE_PACKED_PERCENTAGE;
		//temp_core_perc[i] = (int) buffer[counter];
		//counter +=  SIZE_PACKED_PERCENTAGE;
	}

	int cache_ratio = (int)buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;

    int cpu_stalled = (int)buffer[counter]; //No se usa
    counter += SIZE_PACKED_PERCENTAGE;

    /*iint siblings = (int) buffer[counter];
    counter++;
    vector<int> core_load;
    for(int i = 0; i < siblings; i++){
        int load = (int) buffer[counter];
        counter++;
        core_load.push_back(load);
    }*/


    //SAVE INTO DB
	int io_usage_medio = 0, gpu_usage_medio = 0;
	for (int i = 0; i < n_io_devices; i++) {
		stringstream ss;
		ss << i;
		//error += db_insert_IOdev(ip_add, /*hw_conf.io_dev[i].dev_name*/ss.str().c_str(), io_devices_w_perc[i], io_devices_io_perc[i]);
		io_usage_medio += io_devices_io_perc[i];
	}
	//db_query_all_IO();


	for (int i = 0; i < n_gpu; i++) {
		stringstream ss;
		ss << i;
		//error += db_insert_Gpu(ip_add, /*hw_conf.gpus[i].model_name*/ss.str().c_str(), gpu_temp[i], gpu_memUsage[i], gpu_pw[i], gpu_Usage[i]);
		gpu_usage_medio += gpu_Usage[i];
	}
	//db_query_all_GPU();
	//error += db_insert_coreload(ip_add, core_load);
	//error += db_insert_CR("127.0.0.2", io_usage_medio/n_io_devices, CPUidle_perc, mem_usage_perc, (n_gpu != 0)? gpu_usage_medio/n_gpu : 0);
	//error += db_insert_CR("127.0.0.3", io_usage_medio/n_io_devices, CPUidle_perc+20, mem_usage_perc+20, (n_gpu != 0)? gpu_usage_medio/n_gpu : 0);

	//Quito porque ya va al elasticsearch
    //error += db_insert_CR(ip_add, io_usage_medio/n_io_devices, CPUidle_perc, mem_usage_perc, (n_gpu != 0)? gpu_usage_medio/n_gpu : 0, temp_g, net_g);
	/*db_query_all_CR();*/

	for (int i = 0; i < n_net_devices; i++) {
		stringstream ss;
		ss << i;
		//error += db_insert_Net(ip_add, /*hw_conf.net_interfaces[i].net_name*/ss.str().c_str(), net_devices_usage_perc[i], net_devices_speed[i]);
	}
	//db_query_all_Net();

	for (int i = 0; i < n_core; i++){
		stringstream ss;
		ss << i;
		//error += db_insert_Cores(ip_add, ss.str().c_str(), temp_core[i], pw_cpu[i], temp_core_perc[i]);
	}


	/*Send information to ES*/
    char timestamp[20];
    strftime(timestamp, 20, "%Y/%m/%d %T", localtime(&time_sample));
    char *ip = new char[ip_add.length() + 1];
    strcpy(ip, ip_add.c_str());
    curl_es_now(getElasticSearchAdd(), CPUidle_perc, mem_usage_perc,(n_gpu != 0)? gpu_usage_medio/n_gpu : 0, io_usage_medio/n_io_devices, temp_g, net_g, ip, timestamp);

    /*char cmd [200];
    sprintf(cmd, "{ \"index\" : { \"_index\" : \"myindex\", \"_type\" : \"computerow\" } }\n{\"ip\": \"%s\", \"cpu\": %i, \"ram\": %i, \"gpu\": %i, \"io\": %i, \"temp\": %i, \"net\": %i, \"timestamp\": \"%s\" }\n",ip,CPUidle_perc,mem_usage_perc,(n_gpu != 0)? gpu_usage_medio/n_gpu : 0,io_usage_medio/n_io_devices,temp_g,net_g, timestamp);
    char* json = (char*)malloc(strlen(cmd));
    strcpy(json, cmd);
    strcpy(&es_line[es_wpointer], json);
    es_wpointer += strlen(json);

    if(es_count == 5){
        es_line[es_wpointer] = '\n';
        es_wpointer++;
        es_line[es_wpointer] = '\0'; //eo string
        es_wpointer++;
        curl_es_bulk_now(getElasticSearchAdd(), &es_line[0]);
        es_count = 0;
        es_wpointer=0;
    } else {
        es_count++;
    }
    free(json);
*/


	/*If a hotspot is detected, send notification to flex*/
    /*char * str = (char*)calloc(46, sizeof(char));
    sprintf(str, "%i:%i:%i:%i", mem_usage_perc, CPUidle_perc, net_g, cache_ratio);
    //std::string displayString(str);


	if (CPUidle_perc > 75)
		hotspotNotification(ip_add, CPU_HOT);
	else if (mem_usage_perc > getMemHotValue())
		hotspotNotification(ip_add, MEM_HOT, str);//displayString);
	else if (net_g > getNetHotValue())
		hotspotNotification(ip_add, NET_HOT, str);//displayString);
	else if (cache_ratio > getCacheHotValue())
	    hotspotNotification(ip_add, CACHE_HOT, str);//displayString);

	free(str);*/
	//displayString.clear();



	/*DATABASE TESTS
	db_query_all_IO();
    db_query_all_Net();
    db_query_all_Cores();
    db_query_all_GPU();
    db_query_all_CR();
    db_query_all_Summary();
    db_query_all_Conf();*/

	//db_close();

	return error;
}

/**
	Obtain a sample packed with the samples obtained in the raw packet.
	@param[in] buffer Raw packet containing the monitored samples.
	@param[out]	mon_sample Packed including the data from the raw buffer.
	@returns 0 if no error occurred and -1 if something happened while obtaining the information of the packet. 
*/
int obtain_sample_from_packet(unsigned char * buffer, Packed_sample &mon_sample){
	int error = 0;
	int counter = 0;
	int i =0;

	/*for(int i=0; i<64;i++){
		printf("Packed buffer en la posicion %d contiene %d\n",i,buffer[i]);
	}*/
	
	unsigned char *pointer_buff = NULL;
	//Aboid type of packet
	pointer_buff = &buffer[BTYPE_POS+1];

	
	unsigned char puntero = 1;
	//printf("------------------------------------------------------------modo bit map: %d",buffer[puntero]);

	/************************* Dependiendo del modo de bitmap habra un contenido u otro **************************************/
	int modo_bitmap = buffer[puntero];
	if(modo_bitmap == 0){ //Solo sample
		puntero++;
		pointer_buff += sizeof(unsigned char);
		counter +=sizeof(unsigned char);
#ifdef DAEMON_SERVER_DEBUG
		//cout << "Bitmap mode:  0\n";
#endif

	}else if(modo_bitmap == 1){ //Solo bitmap

		pointer_buff += sizeof(unsigned char);
		counter +=sizeof(unsigned char);
		puntero++;
#ifdef DAEMON_SERVER_DEBUG
		//printf("modo del bitmap es 1\n");
#endif
		int bytes_bitmap = buffer[puntero];
		counter += bytes_bitmap+1;
		//return counter;
		/* Solo se va a recibir el bitmap */		

	}else if(modo_bitmap == 2){ // Bitmap y sample
		
		//pointer_buff += sizeof(unsigned char);
		pointer_buff += 1;
		//counter +=sizeof(unsigned char);
		counter += 1;
		puntero++;
		//Numero de bytes del bitmap
		int bytes_bitmap = buffer[puntero];
#ifdef DAEMON_SERVER_DEBUG
		//printf("numero de bytes del bitmap %d\n", bytes_bitmap);
#endif
		//pointer_buff += sizeof(unsigned char);
		pointer_buff += 1;
		//counter +=sizeof(unsigned char);
		counter += 1;
		puntero++;

		//Bitmap
#ifdef DAEMON_SERVER_DEBUG
		//printf("El bitmap es:\n");
		for(int i = 0; i < bytes_bitmap; i++){
			//printf("bitmap[%d]: %d\n",i,buffer[puntero+i]);
		}
#endif
		//pointer_buff += sizeof(unsigned char) * bytes_bitmap;
		pointer_buff += bytes_bitmap;
		//counter += sizeof(unsigned char) * bytes_bitmap;
		counter += bytes_bitmap;
		puntero+=(unsigned char)bytes_bitmap;

		//printf("modo del bitmap es 2\n");
		
	}
	//puntero++;



	//	exit(0);		
//	counter+= (int) puntero;

	//Obtaining information of time interval 
	pointer_buff += sizeof(unsigned int);
	counter +=sizeof(unsigned int);
	
	time_t time_sample = obtain_time_from_packet(pointer_buff);
	
	#ifdef DAEMON_SERVER_DEBUG
		 char buff[20];
		 strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&time_sample));
		// if(displaySample == 1)
            	//cout << buff << endl;
		 //std::ofstream os;
		 //os.open("/home/alberto/Escritorio/net_log.txt", std::ios::out | std::ios::app);
		 //os << buff << ";1" << std::endl;
		 //os.close();
	#endif
	
	pointer_buff += sizeof(time_t);
	counter +=sizeof(time_t);
	
	//TODO:Should obatin the mon sample complete with the information from the sample
	//Parsing packed: mon sample should already have harware conf and number of samples
	for(i = 0; i < mon_sample.n_samples; ++i){
		 
		#ifdef DAEMON_SERVER_DEBUG
		 	print_raw_packet_sample(pointer_buff, mon_sample.sample_size, mon_sample.n_devices_io, mon_sample.n_interfaces, mon_sample.n_cpu, mon_sample.n_gpus, mon_sample.ip_addr_s);
			//cout << "Sample size: " << mon_sample.sample_size << endl;
		#endif

		//Send data to DB and transform it in json
		error = import_mon_packet_to_database(pointer_buff, mon_sample.n_devices_io, mon_sample.n_interfaces, mon_sample.n_cpu, mon_sample.n_gpus, mon_sample.ip_addr_s, time_sample);

		/*Point to the next sample in the packet*/
		pointer_buff+=mon_sample.sample_size;
		counter+=mon_sample.sample_size;
	}
//	counter = counter + puntero;
	return counter;
}

int create_sample_packet(Packed_sample &info, unsigned char * buffer){


	int contador = 0;
	buffer[contador] = (unsigned char)info.n_samples;
	contador++;
	
	//Include bitmap mode
	//contador+= convert_int_to_char(&buffer[contador],info.modo_bitmap);
	buffer[contador] = (unsigned char)info.modo_bitmap;
	contador++;	


	if(info.modo_bitmap == 0){ //No se envia el bitmap pero si el sample
	
		//Include interval 
		contador+= convert_int_to_char(&buffer[contador],info.interval);
		//Include time 
		contador+= convert_time_to_char(&buffer[contador],info.time_sample);
		
		#ifdef DEBUG_DAEMON
	        	//std::cout<< "contador es: "<< contador<< std::endl;
	    #endif

		/* Ahora los datos no empiezan en esa posición tiene que ser calculada */
		//Empezamos en el 12 que es donde empiezan los datos de las muestras
	
	
		memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.packed_buffer[12], (info.n_samples * info.sample_size));

		#ifdef DEBUG_DAEMON
	    for(int i = 0; i < 33; i++){
			printf("****************%d***************************\n", info.packed_buffer[i]);
		}
	
		for(int i = 0; i < (contador +(info.n_samples * info.sample_size)); i++){
			printf("buffer ------%d------\n",buffer[i]);
		}
	    #endif

	}else if(info.modo_bitmap == 1){ //No se envia el sample pero si el bitmap
	
		//Include bytes of bitmap
		//contador+= convert_int_to_char(&buffer[contador],info.);	
		buffer[contador] = (unsigned char)info.bytes_bitmap;
		contador++;	
	
		//Include bitmap
		memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.bit_map[0], info.bytes_bitmap);
		contador+= (unsigned char)info.bytes_bitmap;

	}else if(info.modo_bitmap == 2){ //Se envia el bitmap y el sample
	
		//Include bytes of bitmap
		//contador+= convert_int_to_char(&buffer[contador],info.);	
		buffer[contador] = (unsigned char)info.bytes_bitmap;
		contador++;
	
		//Include bitmap
		memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.bit_map[0], info.bytes_bitmap);
		contador+= (unsigned char)info.bytes_bitmap;


		//Include interval 
		contador+= convert_int_to_char(&buffer[contador],info.interval);
		//Include time 
		contador+= convert_time_to_char(&buffer[contador],info.time_sample);
	
		#ifdef DEBUG_DAEMON
	        	//std::cout<< "contador es: "<< contador<< std::endl;
	    #endif
		
		/* Se copia el sample en el buffer */
		//Empezamos en el 12 que es donde empiezan los datos de las muestras

		memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.packed_buffer[12], (info.n_samples * info.sample_size));

		#ifdef DEBUG_DAEMON
		for(int i = 0; i < 33; i++){
				printf("****************%d***************************\n", info.packed_buffer[i]);
		}
	
		for(int i = 0; i < (contador +(info.n_samples * info.sample_size)); i++){
				printf("buffer ------%d------\n",buffer[i]);
		}
	    #endif
	
	}



	return contador+info.sample_size;//0;
}

/**
 * This function keeps sending bytes from a buffer until the specified amount
 * is actually sent. It is designed to overcome the problem of send function
 * being able to return having sent only a portion of the specified bytes.
 * @param socket_descriptor
 * @param buf
 * @param n specifies the amount of bytes to send from the first positions of the buffer
 * @param out_addr
 * @return On success n is returned. On error -1 is returned.
 */
int sendn(int socket_descriptor, void *buf, int n, struct sockaddr_in *out_addr) {
	char *buf_ptr;
	int bsent; //actual bytes sent in last send call (may be lower than value specified to send)
	int bleft = n; //bytes left to be sent

	buf_ptr =(char *) buf;
	bsent = sendto(socket_descriptor, buf_ptr, n, 0, (struct sockaddr *) out_addr, sizeof(struct sockaddr_in));
	return n; //success
}

/**
 * This function keeps receiving bytes from a buffer until the specified amount
 * is actually received. It is designed to overcome the problem of recv function
 * being able to return having received only a portion of the specified bytes.
 * @param socket_descriptor
 * @param buf
 * @param n specifies the amount of bytes to receive in the buffer
 * @return On success n is returned. On premature close/shutdown on peer 0 is returned. On error -1 is returned.
 */
int recvn(int socket_descriptor, void *buf, int n) {
	char *buf_ptr;
	int bread; //actual bytes received in las recv call (may be lower than value specified to recv)
	int bleft; //bytes left to be received

	buf_ptr = (char *) buf;
	bleft = n;

	do {
		bread = recv(socket_descriptor, buf_ptr, bleft, 0);
		bleft -= bread;
		buf_ptr += bread; //pointer arithmetic to manage offset
	} while ( (bread > 0) && ( bleft != 0) );

	if ( bread <= 0) { //recv went wrong
		if ( bread == 0){ //peer socket closed/shutdown
			return (0);
		}
		//other error during last recv
		return (-1);
	}
	//else bleft == 0 // all bytes sent
	return n;
}

/**
 * Log monitoring packet into file.
 * @param[in] packet Packet to be logged (received as received from network buffer)
 * @param[in] size Size of the packet (not counting on number of packets)
 * @param[in] clientIP Ip from which the packet was received.
*/
void log_monitoring_packet(unsigned char *packet, ssize_t size, const std::string &clientIP){
	//4 bytes for ipV4 address
	unsigned int IP_add_bin = 0;
	int error = 0;
	const char * c = clientIP.c_str();
	//Locking in case there are several threads working
	std::unique_lock<std::mutex> lock(logging_lock);
	std::ofstream log_file(MON_LOG_FILE, std::ios_base::out | std::ios_base::app );
	
	//Convert string ip address to binary
	error = inet_pton(AF_INET, c, (void * ) &IP_add_bin);
#ifdef DAEMON_SERVER_DEBUG
	//cout << "IP int: " << IP_add_bin << endl;
#endif
	if (error != 1){
		cerr << "An error occured while trying to get the IP."<< endl;
		IP_add_bin = ntohl(IP_add_bin);
	}else{
		log_file.write((const char *) &IP_add_bin, sizeof(unsigned int));
		//Write size of the packet with teh samples and teh hour plus the number of packets (type of packet) max 127
		log_file.write((const char *) packet, size+sizeof(char));
    		log_file << std::endl;
		//File closed by destrcutor
	}
}

/**
 * Processes information provided by Daemons
 * @param buffer
 * @param size
 * @param clientIP
 * @return
 */
int manage_monitoring_packet (unsigned char * buffer, ssize_t size,const std::string &clientIP){
	int error = 0;
	Hw_conf hw_conf;
	int n_samples= 0;
	error = obtain_hw_conf(clientIP, &hw_conf);
	int info_size = 0;
	unsigned int time_interval= 0;
	if(error ==-1){
		//cerr <<  clientIP << "not found..."<< endl;
    }else{
		//print_hw_conf(&hw_conf);
		n_samples = (int) buffer[BTYPE_POS];
		
		#ifdef DAEMON_SERVER_DEBUG
			//cout << "Number of samples to be unpackaged " << n_samples << endl;
			//cout << "size of request" << info_size << endl;
		#endif
		
		//Obtain interval to create packedsample
		time_interval = convert_char_to_int(&buffer[BTYPE_POS+1]);
		Packed_sample sp(hw_conf, time_interval,n_samples, 50);
		//Include interval of time
		info_size = obtain_sample_from_packet(buffer, sp);
		
	}
	//cout << "SIze of packed to be logged " << info_size << endl;
	std::cout <<"Logging..." << std::endl;
	log_monitoring_packet(&buffer[BTYPE_POS], info_size, clientIP);

	//sp = Packed_sample;

	return error;
}

/**
 * Based on the information provided by user (n_processes and profile), it returns the best nodes to run an application
 * @param buffer
 * @param si_other
 * @param socket
 * @return "0" if there isn't info about nodes. "node_name:free_cores"
 */
void * manage_allocate_packet(unsigned char * buffer, struct sockaddr_in si_other, int socket) {
    int np = 2, profile = 1, counter = 0, pointer_final = 0, pairs = 0, index_cadena = 0, no_points = 0;
    char *hostname = (char *) calloc(NI_MAXHOST, sizeof(char));
    char *cadenaFinal = (char *) calloc((np + 4) * (NI_MAXHOST + 3), sizeof(char));
    char *token;
    int ncores = 0, cpu_usage = 0;

    int query = 0;
    char *b = (char *) buffer;
    token = strtok_r(b, ":", &b);
    while (token != NULL) {
        if (strcmp(token, "X") == 0) {
            /*printf("Display next sample ");
            token = strtok_r(b, ":", &b);//token = strtok(NULL, ":");
            printf("with message: %s\n", token);
            query=1;*/
            query = 2;
        } else {
            printf("Query allocate detected\n");
            np = strtol(token, NULL, 10);
            if (np <= 0) {
                /*query = 1;
                token = strtok_r(b, ":", &b);
                char err[] = "ERROR TOK";
                if (token == NULL)
                    token = err;*/
                query = 2;
            } else {
                token = strtok_r(b, ":", &b);//token = strtok(NULL, ":");
                profile = strtol(token, NULL, 10);
                if (profile > 0) {
                    query = 0;
                } else {
                    //query=1;
                    query = 2;
                }
            }
        }
        break;
    }

    if (query == 1) {
        //display information about current state
        displaySample = 1;
        displayedMes = token;

        free(cadenaFinal);
        free(hostname);
    } else if (query == 0) {
        displaySample = 1;   // TO CHECK TEST AND VALIDATION
        char *newBuf = curl_es_get_allocation(getElasticSearchAdd(), np, profile);//db_query_allocate(np, profile);
        //std::cout << "Allocation: " << newBuf << std::endl;
        if (newBuf != NULL) {
            char *auxBuff = (char *) calloc(strlen(newBuf), sizeof(char));
            strcpy(auxBuff, newBuf);
            if (newBuf != NULL) { //crear paquete
                int index = 0;
                char *cadena = (char *) calloc(NI_MAXHOST + 3, sizeof(char));
                for (int i = 0; i < strlen(newBuf) && counter < np; i++) {
                    if (i != 0 && pairs != 1 && no_points == 0) {
                        cadena[index_cadena] = ':';
                        index_cadena++;
                    }
                    no_points = 0;

                    int size;
                    //unsigned int size = (unsigned int) newBuf[index];
                    char *pch = strchr(auxBuff, ':');
                    size = pch - auxBuff;
                    char *merge = (char *) calloc(strlen(pch), sizeof(char));
                    strcpy(merge, pch);
                    memset(auxBuff, 0, sizeof(auxBuff));
                    strcpy(auxBuff, &merge[1]);
                    free(merge);
                    //index++;

                    //to manage SIGSEV exceptions
                    if ((index + size) > strlen(newBuf))
                        break;

                    char aux[size + 1];
                    memcpy(aux, &newBuf[index], size);
                    aux[size] = '\0';
                    index += size + 1;
                    char *end;
                    char *campo = static_cast<char *>(malloc(sizeof(char) * size));
                    memset(campo, 0, size);
                    strcpy(campo, aux);
                    if (pairs == 0) {
                        int err = obtainHostNameByIP(campo, hostname);
                        if (err == 0) {
                            strcpy(&cadena[index_cadena], hostname);
                            index_cadena += strlen(hostname);
                        } else {
                            strcpy(&cadena[index_cadena], campo);
                            index_cadena += strlen(campo);
                        }
                        i = index;
                        pairs++;
                    } else if (pairs == 1) { //cpu usage
                        ncores = strtol(campo, NULL, 10);
                        i = index;
                        pairs++;
                    } else { //num cores
                        cpu_usage = strtol(campo, NULL, 10);
                        i = index;
                        pairs++;
                    }

                    //add to cadena
                    if (pairs == 3) {
                        pairs = 0;
                        int c = ncores - ceil(cpu_usage * ncores / 100.0);
                        if (c < 0) c = 0;
                        counter += c; //send nodes until counter>=np --> min number of nodes in which the app could run
                        if (c != 0) {
                            if (counter > np) //return same free cores than np requested.
                                c -= counter - np;

                            std::string c_s = std::to_string(c);
                            strcpy(&cadena[index_cadena], c_s.c_str());
                            index_cadena += strlen(c_s.c_str());
                            i = index;
                            strcat(cadenaFinal, cadena);
                            free(cadena);
                            cadena = (char *) calloc(NI_MAXHOST + 3, sizeof(char));
                            index_cadena = 0;
                        } else {
                            no_points = 1;
                            free(cadena);
                            cadena = (char *) calloc(NI_MAXHOST + 3, sizeof(char));
                            index_cadena = 0;
                        }
                    }
                    free(campo);
                }
                if (strlen(cadenaFinal) == 0) strcpy(cadenaFinal, "NULL");
                free(cadena);
            } else {
                strcpy(cadenaFinal, "NULL");
            }


            size_t slen = sizeof(si_other);
            //std::cout << cadenaFinal << std::endl;
            int res = sendto(socket, cadenaFinal, strlen((const char *) cadenaFinal), 0, (struct sockaddr *) &si_other,
                             slen);
            if (res < 0)
                printf("Error sending packake\n");
            free(auxBuff);
        }

        /*free(cadenaFinal); done after
        free(hostname);*/
    }
    free(cadenaFinal);
    free(hostname);
}

/**
 * Do a simple query based on the option received
 * @param buffer Buffer that contains the option.
 * @param clientIP IP client
 */
void * manage_query_packet(unsigned char * buffer, struct sockaddr_in si_other, int socket){
    if (buffer[0] == QUERY_CONFIGURATION){
        std::vector<std::vector<std::string>> conf = curl_es_get_conf(getElasticSearchAdd());
        conf = getLastConfMachines(conf);
        unsigned char * newBuf = (unsigned char *)db_query_all_Conf();
        size_t slen = sizeof(si_other);
        int res = sendto(socket, newBuf, strlen((const char *)newBuf), 0, (struct sockaddr *)&si_other, slen);
        if(res < 0)
            printf("Error sending packake\n");

    } else if (buffer[0] == QUERY_STATE_NOW){
        unsigned char * newBuf = (unsigned char *)db_query_all_Now();
        size_t slen = sizeof(si_other);
        int res = sendto(socket, newBuf, strlen((const char *)newBuf), 0, (struct sockaddr *)&si_other, slen);
        if(res < 0)
            printf("Error sending packake\n");

    } /*else if (buffer[0] == QUERY_AVG){
        unsigned char * newBuf = (unsigned char *)db_query_all_Summary();
        size_t slen = sizeof(si_other);
        int res = sendto(socket, newBuf, strlen((const char *)newBuf), 0, (struct sockaddr *)&si_other, slen);
        if(res < 0)
            printf("Error sending packake\n");
    }else if (buffer[0] == QUERY_ONE_NOW){
		char * ip = (char*)malloc(sizeof(char)*15);
		strcpy(ip, (char*)&buffer[2]);
		unsigned char * newBuf = (unsigned char *)db_query_spec_CR(ip, -1, -1, -1, -1);
		free(ip);
		size_t slen = sizeof(si_other);
		if(newBuf != NULL) {
			int res = sendto(socket, newBuf, strlen((const char *) newBuf), 0, (struct sockaddr *) &si_other, slen);
			if (res < 0)
				printf("Error sending packake\n");
		} else {
			//to unlock the client
			int res = sendto(socket, &buffer[0], 1, 0, (struct sockaddr *) &si_other, slen);
		}
	}*/
}

/**
 * it obtain node-name based on its ip (nslookup)
 * @param ip
 * @param host
 * @return
 */
int obtainHostNameByIP(char const * ip, char * host) {

    if (strcmp(ip, "10.0.40.15") == 0) {
        strcpy(host, "compute-11-2");
        return 0;
    } else if (strcmp(ip, "10.0.40.18") == 0) {
        strcpy(host, "compute-11-4");
        return 0;
    } else if (strcmp(ip, "10.0.40.19") == 0) {
        strcpy(host, "compute-11-5");
        return 0;
    } else if (strcmp(ip, "10.0.40.12") == 0) {
        strcpy(host, "compute-11-6");
        return 0;
    } else if (strcmp(ip, "10.0.40.20") == 0) {
        strcpy(host, "compute-11-7");
        return 0;
    } else if (strcmp(ip, "10.0.40.13") == 0) {
        strcpy(host, "compute-11-8");
        return 0;
    } else if (strcmp(ip, "163.117.148.24") == 0) {
        strcpy(host, "arpia.arcos.inf.uc3m.es");
        return 0;
    }else if (strcmp(ip, "127.0.0.1") == 0) {
        strcpy(host, "localhost");
        return 0;
    }
    return 1;

    /*struct sockaddr_in *sa = (sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    socklen_t len;
    char hbuf[NI_MAXHOST];

    memset(&sa->sin_zero, 0, sizeof(sa->sin_zero));

    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = inet_addr(ip);
    len = sizeof(struct sockaddr_in);

    if (getnameinfo((struct sockaddr *) sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)) {
        ///printf("Can't find host\n");
        return 1;
    } else {
        char *newhbuf = (char *) calloc(strlen(hbuf), 1);// - 5, 1);
        memcpy(newhbuf, hbuf, strlen(hbuf));// - 5);
        //printf("host=%s\n", newhbuf);
        strcpy(host, newhbuf);
        free(newhbuf);
        return 0;
    }*/
}

/**
 * Return the minimum of the two ints provided
 * @return min of two ints
 */
int min(int a, int b){
	return (a < b ? a : b);
}

#endif

