#ifndef SYSTEM_FEATURES_H
#define SYSTEM_FEATURES_H

#include <vector>
#include <string>
//#include "daemon_global_params.hpp"
#include "net_info.hpp"
#include "cpu_info.hpp"
#include "devices_info.hpp"
#include "temp_info.hpp"
//#include "gpu_info.hpp"
#include "power_cpu_info.hpp"

#define HW_LOG_FILE "hw_conf_cluster.txt"
#define MON_LOG_FILE "daemon_log_file.txt"

using namespace std;


typedef struct hw_conf {

	/* IP Addres */
	string ip_addr_s;

	/* MAC Addres */
	string mac_addr;

	/* Hostname */
	string hostname;

	/* Memory total */
	int mem_total;

	/* Modo envio bitmap */
	int modo_bitmap;

	/* Number of devices */
	int n_devices_io;

	/* Number of CPUs */
	/* FIXME: arreglar tipos de datos (unsigned char) */
    	int n_cpu;

	/* Number of cores */
	int n_cores;

	/* Number of GPUs */
	int n_gpu;

	/*NUmber of network interfaces*/
	int n_interfaces;

	/* Network interfaces */
	vector<Net_dev> net_interfaces;

	/* CPU stats */
	vector<Cpu_dev> cpus;
	int path_dir;	

	/* GPU CUDA_compatible_devices stats */
	/*int GPU_DEVICES_COMPATIBLE;
	vector<Gpu_dev> gpus;
	void * cuLib;
	void * nvmlLib;*/

	/* IO DEVICES */
	vector<IO_dev> io_dev;

	/* Temperature features */
	vector<Temp_features> temp_features;
	int n_core_temps;

	vector<std::string> vcore_path;
	vector<std::string> files_input;
	vector<std::string> files_max;
	double max_temp;

	/* Power cpu features */
	vector<Power_cpu> pwcpu_features;

}Hw_conf;

//void open_dir();

void print_hw_conf(Hw_conf* hw_conf);


/**
* Logs hardware info to a buffer in binary.
    @param[in] hw_conf Hardware configuration to be logged.
    @param[in, out] buffer Buffer in which the binary information will be logged. Must be already allocated.

*/
int log_hw_conf(Hw_conf* hw_conf, unsigned char * buffer);
//void append_path(std::string & dest, std::string & source);

void get_system_features();


/*
 ***************************************************************************
* Append info (in double precission) to log.
* IN:
* @data		Data to be appended to the log.
* @precis		Decimal precision for the next floating-point value inserted.
 ***************************************************************************
*/
void log_concat(double data, short precis);
void log_append(string toappend);
void log_clear();
void log_concat_interfaces(Hw_conf hw_features);
string get_log_line();
void header_append(string toappend);
string get_header_line();
void log_concat_gpus(Hw_conf hw_features);
void log_concat_coretemps(vector<Temp_features> temp_features);

#endif
