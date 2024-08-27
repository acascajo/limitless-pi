/**
* DaemonMonitor		Provides information about CPU, IO, Networks and GPUs
+ stats in a node.
* @version              v0.1
*/


#ifndef DAEMON_INFO
#define DAEMON_INFO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <time.h>
#include <vector>
#include <chrono>
#include <sys/stat.h>
#include <thread>
#include <dirent.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <iomanip>
#include <algorithm>
#include "system_features.hpp"
#include "common.hpp"
#include "daemon_monitor.hpp"
#include "dir_path.hpp"
#include "daemon_global_params.hpp"
#include "net_info.hpp"
#include "temp_info.hpp"
#include "devices_info.hpp"
#include "cpu_info.hpp"
//#include "gpu_info.hpp"
#include "power_cpu_info.hpp"
#include "memory_info.hpp"
#include "Packed_sample.hpp"
#include "network_data.hpp"
#include "cliente_monitor.hpp"
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <sstream>
#include <limits.h>
#include "connector.h"
//#include "servidor_monitor.cpp"

using namespace std;

/**************************************************************************************************************/

Hw_conf hw_features;

int mode = -1;

Packed_sample *ps;

Packed_sample _last_ps;

int server_socket;

int threshold = 0;
int opt, port = 0;
char *server = NULL, *server2 = NULL, *server3 = NULL, *es_addr;
int error = 0;
int heartbit = 0;
int tmrd = 0;
int top_relation = 0, top_counter = 0;
int net_reducer;
int only_hotspots = 0;
int th_cpu = 0, th_mem = 0, th_io = 0, th_net = 0, th_ener = 0;
std::vector<int> thresholds(5);
std::chrono::milliseconds tmilisleep;

//int modo_bitmap = 0;

int n_devices_io = 0, n_cpu = 0,  n_cores = 0, n_interfaces = 0, n_samples = 0, n_core_temps = 0;

std::vector<int> comparePositions;

unsigned int tinterval = 1;

/*
****************************************************************************************************************
* Prints manual and interpretation of results.
****************************************************************************************************************
*/
void print_manual(){
    	/* TODO: Include correct usage, parameters...*/
    	cout << "Output values description:"<< endl;
    	cout << "   IP_Addr Mem(GB) MemUsage(%) NCPU NCores CPUIdle(%) <for each device> (w(%) tIO(%)) <for each interface> (Speed(Gb/s) NetUsage(%)) <for each core temp> CurrentTemp Temp(%)" << endl;
    	cout << "   IP_Addr: Host ip" << endl;
    	cout << "   Mem: Total dynamic memory in GigaBytes" << endl;
    	cout << "   MemUsage: Percentage of dynamic memory used" << endl;
    	cout << "   NCPU: Number of cpu per node" << endl;
    	cout << "   NCores: Number of cores per node" << endl;
    	cout << "   CPUIdle: percentage of cpu idle in the sampling interval" << endl;
    	cout << "   w: percentage of writes related to IO traffic in the sampling interval" << endl;
    	cout << "   tIO: Percentage of IO time in the sampling interval" << endl;
    	cout << "   Speed: Speed of the interface in Giga bits per second" << endl;
    	cout << "   NetUsage: Percentage of interface used in the sampling interval" << endl;
    	cout << "   Current temp: Current temperature of the node on degrees Celsius." << endl;
    	cout << "   Temp(%): Percentage of temperature reached based on the maximum temperature." << endl;
    	cout << endl;
}

/*
****************************************************************************************************************
* Computes the size of each package from the number of devices, interfaces, and samples that are sent in each packet.
****************************************************************************************************************
*/
void calculate_packed_size(){
    	int packed_size = 0;

    	packed_size = 13 + 2 * (hw_features.n_devices_io + hw_features.net_interfaces.size()+ n_core_temps) + (n_samples - 1 ) * (8 + 2 * (hw_features.n_devices_io + hw_features.net_interfaces.size() + n_core_temps));
	
            #ifdef DEBUG_DAEMON
                    cout << "packed size: " << packed_size << endl;
            #endif


}

void signalHandler(int _signal){

	cout << endl;

	cout << "señal recibida: " << _signal << endl;

	/* Liberación y destrucción de recursos */
	delete ps;

	/*  */
//	freeLibrary(hw_features.cuLib);		

	cout << "liberacion de recursos" << endl;


	exit(_signal);
}

void sigAlarmHandler(int _signal){
 	signal(SIGALRM,sigAlarmHandler);
}

//Setup signal handler for termination 
void setup_signal_term(){
	signal(SIGINT, signalHandler);
        signal(SIGALRM,sigAlarmHandler);

}

/**
 *  Check if the metrics contains any hotspot to notify (only if flag is active)
 * @param ps
 * @return
 */
int checkSendOnlyHS(Packed_sample* ps) {

    int cont = 0;
    int pos = 12; //%mem
    if (ps->packed_buffer[pos] > thresholds[0]) {
        cont++;
        std::cout << "MEM: " << ps->packed_buffer[pos] << " mayor que " << thresholds[0] << std::endl;
    }
    pos++; //%cpu
    if (ps->packed_buffer[pos] > thresholds[1]) {
        cont++;
        std::cout << "CPU: " << ps->packed_buffer[pos] << " mayor que " << thresholds[1] << std::endl;
    }
    pos++; // points to energy
    if (ps->packed_buffer[pos] > thresholds[2]) {
        cont++;
        std::cout << "ENERGY: " << ps->packed_buffer[pos] << " mayor que " << thresholds[2] << std::endl;
    }
    for (int a = 0; hw_features.n_devices_io > a; a++) {
        pos += 2;
        if (ps->packed_buffer[pos] > thresholds[3]) {
            cont++;
            std::cout << "IO: " << ps->packed_buffer[pos] << " mayor que " << thresholds[3] << std::endl;
        }
    }
    for (int a = 0; hw_features.n_interfaces > a; a++) {
        pos += 2;
        if (ps->packed_buffer[pos] > thresholds[4]) {
            cont++;
            std::cout << "NET: " << ps->packed_buffer[pos] << " mayor que " << thresholds[4] << std::endl;
        }
    }


    return cont;
}

/**
 * If sample values are in a determined range, it won't be sended (to reduce net usage)
 * @param ps
 * @return 0 if sample must be sended, 1 if not.
 */
int checkRangePS(Packed_sample* ps){
    int cont = 0;
    int i = 8;
    if (comparePositions.size() == 0){
        int pos = 12;
        comparePositions.push_back(pos);
        pos++;
        comparePositions.push_back(pos);
        pos++; // points to energy
        for (int a = 0; hw_features.n_devices_io > a; a++){
            pos+=2;
            comparePositions.push_back(pos);
        }
        for (int a = 0; hw_features.n_interfaces > a; a++){
            pos+=2;
            comparePositions.push_back(pos);
        }
        pos += 2*hw_features.n_core_temps;
        pos++;
        comparePositions.push_back(pos);
        pos++;
        comparePositions.push_back(pos);
    }

    for (auto i : comparePositions){//i; i < ps->packed_ptr; i++){
        if ((_last_ps.packed_buffer[i] + threshold) < ps->packed_buffer[i] || (_last_ps.packed_buffer[i] - threshold) > ps->packed_buffer[i])
            cont++;
    }

    if (cont > 0) {
        cont = 1;
        memcpy(&_last_ps.packed_buffer, ps->packed_buffer, sizeof(ps->packed_buffer));
    }
    return cont;
}

/**
 * This function executes a command in linux returning the output.
 * @param cmd
 * @return
 */
std::string execCommand(const char * cmd){
    std::string res = "";
    std::array<char,128> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe){
        throw std::runtime_error("popen() failed");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
        res += buffer.data();
    }

    return res;
}

/**
 * Update internal params from a file edited by user
 */
void updateConfParams(){

    std::cout << "\n********** Updating parameters *************" << std::endl;
    std::ifstream initfile(CONF_FILE);
    int error = 0;
    std::string line;
    int param_n = 0;
    int sizet = 0;
    while (std::getline(initfile, line))
    {
        if (line.length() > 0 && line.at(0) != '#') {
            switch (param_n) {
                case 0: // interval time
                    tinterval = strtol(line.c_str(), NULL, 10);
                    tmilisleep = std::chrono::milliseconds(tinterval);
                    printf("Interval time: %i\n", tinterval);
                    param_n++;
                    break;
                case 1: //port
                    port = strtol(line.c_str(), NULL, 10);
                    printf("Port: %i\n", port);
                    param_n++;
                    break;
                case 2: //num samples
                    n_samples = strtol(line.c_str(), NULL, 10);
                    printf("Num samples: %i\n", n_samples);
                    param_n++;
                    break;
                case 3: // server ip
                    sizet = strlen(line.c_str());
                    server = (char *) malloc(sizet);
                    strcpy(server, line.c_str());
                    printf("Server ip: %s\n", server);
                    param_n++;
                    break;
                case 4: // ES ip
                    sizet = strlen(line.c_str());
                    es_addr = (char *) malloc(sizet);
                    strcpy(es_addr, line.c_str());
                    printf("ElasticSearch IP: %s\n", es_addr);
                    param_n++;
                    break;
                case 5: // bitmap mode (0 off, 1 on)
                    hw_features.modo_bitmap = strtol(line.c_str(), NULL, 10);
                    cout << "Bitmap: " << hw_features.modo_bitmap << endl;
                    param_n++;
                    break;
                case 6: // threshold filter (0 no, 1 yes)
                    net_reducer = strtol(line.c_str(), NULL, 10);
                    printf("Threshold filter: %i\n", net_reducer);
                    param_n++;
                    break;
                case 7: //threshold filter value
                    threshold = strtol(line.c_str(), NULL, 10);
                    printf("Threshold filter value: %i\n", threshold);
                    param_n++;
                    break;
                case 8: // top interval
                    top_relation = strtol(line.c_str(), NULL, 10);
                    printf("Interval TOP: %i\n", top_relation);
                    param_n++;
                    break;
                case 9: // TMR (0 simple, 1 triple)
                    tmrd = strtol(line.c_str(), NULL, 10);
                    printf("TMR mode: %i\n", tmrd);
                    param_n++;
                    break;
                case 10: // backup IP 1
                    sizet = strlen(line.c_str());
                    server2 = (char *) malloc(sizet);
                    strcpy(server2, line.c_str());
                    if (strcmp(server2, "-1") == 0) server2 = NULL;
                    printf("Server backup 1: %s\n", server2);
                    param_n++;
                    break;
                case 11: // backup IP 2
                    sizet = strlen(line.c_str());
                    server3 = (char *) malloc(sizet);
                    strcpy(server3, line.c_str());
                    if (strcmp(server3, "-1") == 0) server3 = NULL;
                    printf("Server backup 2: %s\n", server3);
                    param_n++;
                    break;
                case 12:
                    only_hotspots = strtol(line.c_str(), NULL, 10);
                    if (only_hotspots != 0)
                        printf("Notify only hotspots.\n");
                    else
                        printf("Notify all.\n");
                    param_n++;
                    break;
                case 13:
                    th_mem = strtol(line.c_str(), NULL, 10);
                    printf("Threshold MEM: %i\n", th_mem);
                    param_n++;
                    break;
                case 14:
                    th_cpu = strtol(line.c_str(), NULL, 10);
                    printf("Threshold CPU: %i\n", th_cpu);
                    param_n++;
                    break;
                case 15:
                    th_io = strtol(line.c_str(), NULL, 10);
                    printf("Threshold IO: %i\n", th_io);
                    param_n++;
                    break;
                case 16:
                    th_net = strtol(line.c_str(), NULL, 10);
                    printf("Threshold NET: %i\n", th_net);
                    param_n++;
                    break;
                case 17:
                    th_ener = strtol(line.c_str(), NULL, 10);
                    printf("Threshold Energy: %i\n", th_ener);
                    param_n++;
                    break;
                case 18:
                    printf("More lines than expected in init.dae file.\n");
                    break;
                default:
                    error = 1;
                    break;
            }
        }

    }

    if (error == 0) {
        //update thresholds
        thresholds[0] = th_mem;
        thresholds[1] = th_cpu;
        thresholds[2] = th_ener;
        thresholds[3] = th_io;
        thresholds[4] = th_net;

        error = init_socket(server, port);
        if (error == -1) {
            std::cerr << "Socket could not be initialized" << endl;
        }
        if (server2 != NULL) {
            error = init_socket_backup1(server2, port);
            if (error == -1) {
                std::cerr << "Backup 1 Socket could not be initialized" << endl;
            }
        }
        if (server3 != NULL) {
            error = init_socket_backup2(server3, port);
            if (error == -1) {
                std::cerr << "Backup 2 Socket could not be initialized" << endl;
            }
        }
    }
}


int main(int argc, char *argv[]) {

    using namespace std::chrono;
    using clk = high_resolution_clock;
    int i = -1;
    FILE *fp;
    clk::time_point t1;
    clk::time_point t2;
    CURL *curl;


#ifdef DEBUG_TIME
    clk::time_point tm1;
            clk::time_point  tm2;
            clk::time_point tc1;
            clk::time_point  tc2;
            clk::time_point td1;
            clk::time_point  td2;
            clk::time_point tn1;
            clk::time_point  tn2;
            clk::time_point tg1;
            clk::time_point  tg2;
            clk::time_point tt1;
            clk::time_point  tt2;
            clk::time_point  tp1;
            clk::time_point  tp2;

#endif

    clk::duration difft;
    clk::duration difference;

    stringstream sargv;
    string sarg;

    if (argc != 1) {
        cerr << "Usage: ./DaeMon [params configured in init.dae file]" << endl;
        exit(0);
    }

    std::ifstream initfile(INIT_FILE);

    std::string line;
    int param_n = 0;
    int sizet = 0;
    while (std::getline(initfile, line))
    {
        if (line.length() > 0 && line.at(0) != '#') {
            switch (param_n) {
                case 0: // interval time
                    tinterval = strtol(line.c_str(), NULL, 10);
                    printf("Interval time: %i\n", tinterval);
                    param_n++;
                    break;
                case 1: //port
                    port = strtol(line.c_str(), NULL, 10);
                    printf("Port: %i\n", port);
                    param_n++;
                    break;
                case 2: //num samples
                    n_samples = strtol(line.c_str(), NULL, 10);
                    printf("Num samples: %i\n", n_samples);
                    param_n++;
                    break;
                case 3: // server ip
                    sizet = strlen(line.c_str());
                    server = (char *) malloc(sizet);
                    strcpy(server, line.c_str());
                    printf("Server ip: %s\n", server);
                    param_n++;
                    break;
                case 4: // ES ip
                    sizet = strlen(line.c_str());
                    es_addr = (char *) malloc(sizet);
                    strcpy(es_addr, line.c_str());
                    printf("ElasticSearch IP: %s\n", es_addr);
                    param_n++;
                    break;
                case 5: // bitmap mode (0 off, 1 on)
                    hw_features.modo_bitmap = strtol(line.c_str(), NULL, 10);
                    cout << "Bitmap: " << hw_features.modo_bitmap << endl;
                    param_n++;
                    break;
                case 6: // threshold filter (0 no, 1 yes)
                    net_reducer = strtol(line.c_str(), NULL, 10);
                    printf("Threshold filter: %i\n", net_reducer);
                    param_n++;
                    break;
                case 7: //threshold filter value
                    threshold = strtol(line.c_str(), NULL, 10);
                    printf("Threshold filter value: %i\n", threshold);
                    param_n++;
                    break;
                case 8: // top interval
                    top_relation = strtol(line.c_str(), NULL, 10);
                    printf("Interval TOP: %i\n", top_relation);
                    param_n++;
                    break;
                case 9: // TMR (0 simple, 1 triple)
                    tmrd = strtol(line.c_str(), NULL, 10);
                    printf("TMR mode: %i\n", tmrd);
                    param_n++;
                    break;
                case 10: // backup IP 1
                    sizet = strlen(line.c_str());
                    server2 = (char *) malloc(sizet);
                    strcpy(server2, line.c_str());
                    if (strcmp(server2, "-1") == 0) server2 = NULL;
                    printf("Server backup 1: %s\n", server2);
                    param_n++;
                    break;
                case 11: // backup IP 2
                    sizet = strlen(line.c_str());
                    server3 = (char *) malloc(sizet);
                    strcpy(server3, line.c_str());
                    if (strcmp(server3, "-1") == 0) server3 = NULL;
                    printf("Server backup 2: %s\n", server3);
                    param_n++;
                    break;
                case 12:
                    only_hotspots = strtol(line.c_str(), NULL, 10);
                    if (only_hotspots != 0)
                        printf("Notify only hotspots.\n");
                    else
                        printf("Notify all.\n");
                    param_n++;
                    break;
                case 13:
                    th_mem = strtol(line.c_str(), NULL, 10);
                    printf("Threshold MEM: %i\n", th_mem);
                    param_n++;
                    break;
                case 14:
                    th_cpu = strtol(line.c_str(), NULL, 10);
                    printf("Threshold CPU: %i\n", th_cpu);
                    param_n++;
                    break;
                case 15:
                    th_io = strtol(line.c_str(), NULL, 10);
                    printf("Threshold IO: %i\n", th_io);
                    param_n++;
                    break;
                case 16:
                    th_net = strtol(line.c_str(), NULL, 10);
                    printf("Threshold NET: %i\n", th_net);
                    param_n++;
                    break;
                case 17:
                    th_ener = strtol(line.c_str(), NULL, 10);
                    printf("Threshold Energy: %i\n", th_ener);
                    param_n++;
                    break;
                case 18:
                    printf("More lines than expected in init.dae file.\n");
                    break;
                default:
                    return -1;
            }
        }
    }

    /*while ((opt = getopt(argc, argv, "hds:p:i:t:a:n:m:b:z:k:e:l:")) != -1) {

        switch (opt) {
            case 'h':
                cout
                        << "./DaeMon -i <time interval (ms)> -s <num_samples> -t <threshold 0-100> -p <number port of server (1024 <= port <= 65535)> -a <address of server in dot format(x.x.x.x)> -b <enable-bitmap 1 - 0> - n <enable-net-reducer 0 - 1>"
                        << endl;

                exit(0);
                break;

            case 'b':
                hw_features.modo_bitmap = strtol(optarg, NULL, 10);
                cout << "Recibido el parametro de bitmap: " << hw_features.modo_bitmap << endl;
                break;

            case 'i':
                tinterval = strtol(optarg, NULL, 10);
                break;
            case 'l':
                top_relation = strtol(optarg, NULL, 10);
                break;

            case 'p':
                port = strtol(optarg, NULL, 10);
                break;

            case 't':
                threshold = strtol(optarg, NULL, 10);
                break;

            case 's':
                n_samples = strtol(optarg, NULL, 10);
                if (n_samples > MAX_SAMPLES) {
                    cerr << "Maximum number of samples allowed 127" << endl;
                    exit(0);
                }
                break;

            case 'a':
                server = optarg;
                printf("Server %s\n", server);
                break;
            case 'z':
                server2 = optarg;
                printf("Server backup 1 %s\n", server2);
                break;
            case 'k':
                server3 = optarg;
                printf("Server backup 2 %s\n", server3);
                break;
            case 'n':
                net_reducer = strtol(optarg, NULL, 10);
                printf("Net reducer: %i\n", net_reducer);
                break;
            case 'm':
                tmr = strtol(optarg, NULL, 10);
                printf("TMR mode: %i\n", tmr);
                break;
            case 'e':
                es_addr = optarg;
                printf("ElasticSearch Ip: %s\n", es_addr);
                break;
            case '?':
                if ((optopt == 'p')) {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (optopt == 'a') {
                    cerr << "ERROR: Option -" << optopt << "requires an argument." << std::endl;
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
            default:
                return -1;
        }
    }*/

    //exit(0);

    /*Convert miliseconds int to class miliseconds*/
    //tinterval += rand() % 2;
    tmilisleep = std::chrono::milliseconds(tinterval);
    thresholds[0] = th_mem;
    thresholds[1] = th_cpu;
    thresholds[2] = th_ener;
    thresholds[3] = th_io;
    thresholds[4] = th_net;

    /*switch (mode) {
        case 0:
            //init_server(4000);
            break;
        case 1:
            //server_socket = init_socket("0.0.0.0","4000");
            break;
        default:
            break;
    }*/

    /*Setup for signal termination */
    setup_signal_term();
    /* Read number of processor */
    error = read_n_processors(hw_features.cpus, hw_features.n_cpu, hw_features.n_cores);//, hw_features.n_siblings);
    
    if (error != EOK) {
        cerr << "Error: " << error << endl;
        exit(0);
    }
#ifdef DEBUG_DAEMON
    cout << "Log line: " << get_log_line() << endl;
    cout << "obteniendo ip del nodo"<< endl;
#endif

    /* Get Ip Address */
    get_addr(hw_features.ip_addr_s, hw_features.hostname);
    cout << "hostname es: " << hw_features.hostname << endl;

    /* Get temp path */
    error = get_temp_path(hw_features.temp_features, hw_features.n_core_temps);
    //	if(error != EOK){
    //		cerr << "Error: "<< error << endl;
    //		exit(0);
    //	}

    /* Get memory total */
    get_mem_total(hw_features.mem_total);

#ifdef DEBUG_DAEMON
    cout << "Log line: " << get_log_line() << endl;
#endif

    header_append("IP_Addr Mem(GB) MemUsage(%) NCPU NCores CPUBusy(%)");

    /* Get power path */
    error = get_power_path(hw_features.pwcpu_features, hw_features.path_dir, hw_features.n_cpu);
//		if(error != EOK){
//			cerr << "Error obteniendo la carpeta de la energia de la cpu: "<< error << endl;
//			exit(0);
//		}


    /* Read number of devices */
    error = read_n_devices(hw_features.io_dev, hw_features.n_devices_io);
    if (error != EOK) {
        cerr << "Error: " << error << endl;
        exit(0);
    }
    header_append(" ");

    error = read_n_net_interface(hw_features.net_interfaces, hw_features.n_interfaces);
    if (error != EOK) {
        cerr << "Error: " << error << endl;
        exit(0);
    }
#ifdef DEBUG_DAEMON
    cout << "Log line: " << get_log_line() << endl;
#endif


    log_concat_interfaces(hw_features);

    log_concat_coretemps(hw_features.temp_features);

    n_core_temps = hw_features.temp_features.size();

#ifdef DEBUG_DAEMON
    cout << "Log line: " << get_log_line() << endl;
    cout << "leyendo el numero de gpus" << endl;
#endif

    /*error = read_n_gpu(hw_features.gpus, hw_features.n_gpu, hw_features.GPU_DEVICES_COMPATIBLE, &hw_features.cuLib,
                       &hw_features.nvmlLib);
    //cout << "El numero de gpus es: " << hw_features.n_gpu << endl;
    if (error == EGPU) {
        hw_features.GPU_DEVICES_COMPATIBLE = CUDA_NO_COMPATIBLE;
    }

    log_concat_gpus(hw_features);*/
    ps = new Packed_sample(hw_features, tinterval, n_samples, threshold);

    /* Print on the screen with the output format of the data. */
    cout << get_header_line() << endl;

    calculate_packed_size();
    

    //INIT Communications: Check that everything is correct
    if (server == NULL) {
        std::cerr << "ERROR: NO address for master was given." << std::endl;
        exit(-1);
    }
    if ((port < 1024) || (port > 65535)) {
        fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535");
        exit(-1);
    }


    error = init_socket(server, port);
    if (error == -1) {
        std::cerr << "Socket could not be initialized" << endl;
        return error;
    }
    if (server2 != NULL) {
        error = init_socket_backup1(server2, port);
        if (error == -1) {
            std::cerr << "Backup 1 Socket could not be initialized" << endl;
            return error;
        }
    }
    if (server3 != NULL) {
        error = init_socket_backup2(server3, port);
        if (error == -1) {
            std::cerr << "Backup 2 Socket could not be initialized" << endl;
            return error;
        }
    }


    //Send initial packet with the configuration
    send_conf_packet(&hw_features);

    long last_conf_update = -1;

    while (1) {
        

        t1 = clk::now();


        /************************* CHECK IF THERE IS A CONFIGURATION UPDATE*************/

        struct stat filestat;
        if(stat(CONF_FILE, &filestat)==0) //si se ha creado
        {
            if (last_conf_update == -1) { //procesar directamente
                last_conf_update = filestat.st_mtime;
                updateConfParams();
            }
            else //comparar primero si hay actualización
            {
                auto t = filestat.st_mtime;
                if (t > last_conf_update){
                    last_conf_update = filestat.st_mtime;
                    updateConfParams();
                }
            }

        }

        /******************************************************************************/


        /* Clear log_line */
        log_clear();

        /* Concat ip address */
        log_append(hw_features.ip_addr_s);



        /*************************  MEMORY USAGE ******************/
#ifdef DEBUG_DAEMON
        cout << "read memory stats 1" << endl;
#endif

#ifdef DEBUG_TIME
        tm1 = clk::now();
#endif

        error = read_memory_stats();
        if (error != EOK) {
            cerr << "Error: " << error << endl;
            exit(0);
        }

#ifdef DEBUG_DAEMON
        cout << "Log line: " << get_log_line() << endl;
#endif


#ifdef DEBUG_TIME
        tm2 = clk::now();
        difft = (tm2 - tm1);
        cout << "Duracion de las operaciones de memoria "<<  std::chrono::duration_cast<std::chrono::microseconds>(difft).count()<<" usec."<< endl;
#endif

#ifdef DEBUG_DAEMON
        cout << "read memory stats 2" << endl;
#endif
        
        /*************************** CPU USAGE ************************/

#ifdef DEBUG_DAEMON
        cout << "read cpu stats 1" << endl;
#endif


#ifdef DEBUG_TIME
        tc1 = clk::now();
#endif
        
        //hw_features.cores.clear();
        read_cpu_stats(hw_features.cpus/*, hw_features.cores*/, hw_features.n_cpu, hw_features.n_cores);



#ifdef DEBUG_DAEMON
        cout << "Log line: " << get_log_line() << endl;
#endif

#ifdef DEBUG_TIME
        tc2 = clk::now();
        difft = (tc2 - tc1);
        cout << "Duracion de las operaciones de cpu "<<  std::chrono::duration_cast<std::chrono::microseconds>(difft).count()<<" usec."<< endl;
#endif


#ifdef DEBUG_DAEMON
        cout << "read cpu stats 2" << endl;
#endif


        /*************************** POWER USAGE **************************/

#ifdef DEBUG_DAEMON
        cout << "read power stats 1" << endl;
#endif

#ifdef DEBUG_TIME
        tp1 = clk::now();
#endif

        get_power(hw_features.pwcpu_features, hw_features.path_dir, hw_features.n_cpu);

#ifdef DEBUG_DAEMON
        cout << "Log line: " << get_log_line() << endl;
#endif

#ifdef DEBUG_TIME
        tp2 = clk::now();
        difft = (tp2 - tp1);
        cout << "Duracion de las operaciones de energia "<<  std::chrono::duration_cast<std::chrono::microseconds>(difft).count()<<" usec."<< endl;
#endif


#ifdef DEBUG_DAEMON
        cout << "read power stats 2" << endl;
#endif

        /************************** DEVICES USAGE ************************/

#ifdef DEBUG_DAEMON
        cout << "read devices stats 1" << endl;
#endif

#ifdef DEBUG_TIME
        td1 = clk::now();
#endif

        read_devices_stats(hw_features.io_dev, tinterval);

#ifdef DEBUG_DAEMON
        cout << "Log line: " << get_log_line() << endl;
#endif

#ifdef DEBUG_TIME
        td2 = clk::now();
        difft = (td2 - td1);
        cout << "Duracion de las operaciones de dispositivos "<<  std::chrono::duration_cast<std::chrono::microseconds>(difft).count()<<" usec."<< endl;
#endif


#ifdef DEBUG_DAEMON
        cout << "read devices stats 2" << endl;
#endif

        /**************************** NET USAGE ******************************/
#ifdef DEBUG_DAEMON
        cout << "read net stats 1" << endl;
#endif

#ifdef DEBUG_TIME
        tn1 = clk::now();
#endif

        read_net_stats(hw_features.net_interfaces);

#ifdef DEBUG_DAEMON
        cout << "Log line: " << get_log_line() << endl;
#endif

#ifdef DEBUG_TIME
        tn2 = clk::now();
        difft = (tn2 - tn1);
        cout << "Duracion de las operaciones de red "<<  std::chrono::duration_cast<std::chrono::microseconds>(difft).count()<<" usec."<< endl;
#endif


#ifdef DEBUG_DAEMON
        cout << "read net stats 2" << endl;
#endif





        /**************************** GET TEMPERATURE ******************************/
#ifdef DEBUG_DAEMON
        cout << "read temperature stats 1" << endl;
#endif

#ifdef DEBUG_TIME
        tt1 = clk::now();
#endif

        get_temperature(hw_features.temp_features);

#ifdef DEBUG_DAEMON
        cout << "Log line: " << get_log_line() << endl;
#endif


#ifdef DEBUG_TIME
        tt2 = clk::now();
        difft = (tt2 - tt1);
        cout << "Duracion de las operaciones de temperatura "<<  std::chrono::duration_cast<std::chrono::microseconds>(difft).count()<<" usec."<< endl;
#endif


#ifdef DEBUG_DAEMON
        cout << "read temperature stats 2" << endl;
#endif

        /**************************** GPU USAGE ******************************/



        /*************************** INFINIBAND ************************************/





        /**************************** CACHE MISSES ******************************/
        int enable_cache = 1;
        double cache_refs = 0.0, cache_misses = 0.0, cpi = 0.0, cycles = 0.0;
        if (enable_cache == 0) {
            //std::string sudo = execCommand("echo passwd | sudo --stdin ls");
            //std::string result = execCommand("perf stat -a -e LLC-loads,LLC-load-misses,LLC-stores,LLC-store-misses,LLC-prefetch-misses sleep 1 2> file 2>&1");
            std::string result = execCommand(
                    "perf stat -a -e cache-references,cache-misses,instructions,cycles sleep 2 2> file 2>&1");
            execCommand("rm file");

            std::vector<std::string> svec = split(result, '\n');
            std::vector<std::string> aux = split(svec[1], ' ');
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), '.'), aux[0].end());
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), ','), aux[0].end());
            cache_refs = strtol(aux[0].c_str(), NULL, 10);
            aux = split(svec[2], ' ');
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), '.'), aux[0].end());
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), ','), aux[0].end());
            cache_misses = strtol(aux[0].c_str(), NULL, 10);
            aux = split(svec[3], ' ');
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), '.'), aux[0].end());
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), ','), aux[0].end());
            cpi = strtol(aux[0].c_str(), NULL, 10);
            aux = split(svec[4], ' ');
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), '.'), aux[0].end());
            aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), ','), aux[0].end());
            cycles = strtol(aux[0].c_str(), NULL, 10);
            /*if (aux[0] != "<not") {
                aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), '.'), aux[0].end());
                aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), ','), aux[0].end());
                cycles = strtol(aux[0].c_str(), NULL, 10);
                aux = split(svec[4], ' ');
                aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), '.'), aux[0].end());
                aux[0].erase(std::remove(aux[0].begin(), aux[0].end(), ','), aux[0].end());
                cpu_cycles = strtol(aux[0].c_str(), NULL, 10);
            }*/
        }

        /*result = "";
        result = accumulate(begin(svec), end(svec), result);*/

        /*cout.precision(4);
        cout << get_log_line() << endl;*/

        if (i != -1) {

#ifdef DEBUG_DAEMON
            //cout << "empaquetando la muestra" << endl;
#endif


            //cout << "Caché misses: " << (cache_misses / cache_refs) * 100 << "%\nCPU stalled: " << stalled << "%" << endl;
            if (enable_cache == 0) {
                double stalled = (cpi / cycles) * 100;
                stalled = (stalled > 100) ? 0 : 100 - stalled;
                ps->pack_sample_s(get_log_line(), (cache_misses / cache_refs) * 100, stalled);//, hw_features.cores);
                ps->packed_ptr++;
            } else {
                ps->pack_sample_s(get_log_line(), 0, 0);//, hw_features.cores);
                ps->packed_ptr++;
            }

/*#ifdef DEBUG_DAEMON
            cout << "Log line: " << get_log_line() << endl;
#endif*/
            i++;

            if (i == n_samples) {
                if(only_hotspots <= 0) {
                    i = 0;
                    /* create packet and send */
                    if (net_reducer == 1) {
                        if (_last_ps.ip_addr_s == "" || heartbit == 10) {
                            heartbit = 0;
                            _last_ps.ip_addr_s = ps->ip_addr_s;
                            memcpy(&_last_ps.packed_buffer, ps->packed_buffer, sizeof(ps->packed_buffer));
                            send_monitor_packet(*ps, tmrd);
#ifdef DEBUG_DAEMON
                            cout << "Log line: " << get_log_line() << endl;
#endif
                        } else if (checkRangePS(ps) != 0) {
                            send_monitor_packet(*ps, tmrd);
#ifdef DEBUG_DAEMON
                            cout << "Log line: " << get_log_line() << endl;
#endif
                        } else
                            heartbit++;
                    } else {
                        send_monitor_packet(*ps, tmrd);
#ifdef DEBUG_DAEMON
                        cout << "Log line: " << get_log_line() << endl;
#endif
                    }
                    //create_sample_packet(ps,packed_buffer);
                    //sendn(server_socket, packed_buffer, sizeof(packed_buffer));
                }
                else
                {
                    //Send only if there is values over the threshold defined by the user
                    if (checkSendOnlyHS(ps) > 0) {
                        /* create packet and send */
                        /*if (net_reducer == 1) {
                            if (_last_ps.ip_addr_s == "" || heartbit == 10) {
                                heartbit = 0;
                                _last_ps.ip_addr_s = ps->ip_addr_s;
                                memcpy(&_last_ps.packed_buffer, ps->packed_buffer, sizeof(ps->packed_buffer));
                                send_monitor_packet(*ps, tmrd);
                            } else if (checkRangePS(ps) != 0)
                                send_monitor_packet(*ps, tmrd);
                            else
                                heartbit++;
                        } else*/
                            send_monitor_packet(*ps, tmrd);
#ifdef DEBUG_DAEMON
                        cout << "Log line: " << get_log_line() << endl;
#endif
                    }
                }
            }
            /*** LINEA DE ESCRITURA Y MUESTRA DE EJECUCIÓN ***/
            cout << "Log line: " << get_log_line() << endl;

#ifdef DEBUG_DAEMON
            cout << "muestra empaquetada" << endl;
#endif
        }

        /*** TOP EXECUTION TO GET INFORMATION ABOUT PROCESSES ***/

        if(top_relation > 0) {
            /*if(top_counter >= top_relation-1) {

                std::array<char, 1024> buffer;
                std::string result;
                std::unique_ptr<FILE, decltype(&pclose)> pipe(
                        popen("top -b -n 1| grep -v \"root\" | grep -v \"0,0\"", "r"),
                        pclose);
                if (!pipe) {
                    throw std::runtime_error("popen() failed!");
                }
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                    result += buffer.data();
                }

                std::vector<std::vector<char *>> lines;
                std::vector<char *> aux;
                char *b = const_cast<char *>(result.c_str());
                char *token, *tokaux;
                for (int j = 0; j < 5; j++)
                    token = strtok_r(b, "\n", &b);

                while (token != NULL) {
                    token = strtok_r(b, "\n", &b);
                    if (token != NULL) {
                        for (int k = 0; k < 8; k++)
                            tokaux = strtok_r(token, " ", &token);
                        tokaux = strtok_r(token, " ", &token); //cpu
                        aux.push_back(tokaux);
                        tokaux = strtok_r(token, " ", &token); //mem
                        aux.push_back(tokaux);
                        tokaux = strtok_r(token, " ", &token);
                        tokaux = strtok_r(token, "\n", &token); //command
                        aux.push_back(tokaux);
                        lines.push_back(aux);
                        aux.clear();
                    }
                }

                time_t rawtime;
                struct tm *timeinfo;
                char raw[80];
                //esto es lo que da problema a veces al compilar y no se por que
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(raw, 80, "%Y/%m/%d %T", timeinfo);
                puts(raw);
                char *timestamp = (char *) malloc(19);
                strcpy(timestamp, raw);
                char *ip = (char*)malloc(hw_features.ip_addr_s.length() + 1);
                strcpy(ip, hw_features.ip_addr_s.c_str());
                for (int j = 0; j < lines.size(); j++) {
                    curl_es_processes(es_addr, atoi(lines[j][0]), atoi(lines[j][1]), lines[j][2],
                                      ip, timestamp);
                }
                free(ip);
                lines.clear();
                free(timestamp);
            }
            top_counter++;
            top_counter %= top_relation;
		*/
        }
        /*********************end top *************************/

        t2 = clk::now();

        difft = (t2 - t1);

        if (difft < tmilisleep) {
            std::chrono::milliseconds randomize(0);//rand() % 2000);
            clk::duration difference = (tmilisleep - difft - randomize);

#ifdef DEBUG_TIME
            cout << "Duration to sleep "<<  std::chrono::duration_cast<std::chrono::microseconds>(difference).count()<<". \n";
            cout << "Monitoring loop duration "<<  (tinterval*1000) - std::chrono::duration_cast<std::chrono::microseconds>(difference).count()<<" usec. \n";
#endif


            struct timeval tval;
            tval.tv_sec = std::chrono::duration_cast<std::chrono::microseconds>(difference).count() / 1000000;
            tval.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(difference).count() % 1000000;

            struct itimerval tival;
            tival.it_value = tval;
            tival.it_interval.tv_sec = 0;
            tival.it_interval.tv_usec = 0;

            setitimer(ITIMER_REAL, &tival, NULL);
            pause();

        } else {

#ifdef DEBUG_TIME
            cout << " Tiempo desbordado" << endl;
#endif

        }
        if (i < 0) { i++; }
    }

    return 0;
}

#endif
