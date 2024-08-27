#ifndef DAEMON_MONITOR_H
#define DAEMON_MONITOR_H


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
#include  <dirent.h>

using namespace std;

/*********************************************ERRORS CODE******************************************************/
enum Daemon_error{	
	EOK = 0, 	/* All work corectly */
	EGEN = -1, 	/* General error */
	EFILE = -2, 	/* Error reading a file */
	EDIR = -3,  	/* Error opening and reading a directory */
	EGPU = -4,
	EDIRCPU = -5 	/* There is no directory to read power stats of the cpu*/
};

/**************************************************************************************************************/
#define BUFF_SIZE 1024
#define MAX_NAME_LEN 128
#define MAX_SAMPLES 127

/*CODES TO SENT PACKETS*/
#define CONFIG_PACKET 0
#define QUERY_CONFIGURATION 255
#define QUERY_STATE_NOW 254
#define QUERY_AVG 253
#define QUERY_ONE_NOW 252

/*OTHER CODES TO DB*/
#define QUERY_PACKET_SIZE 1
#define QUERY_PACKET_SPECT 17
#define QUERY_ALLOCATE_SIZE 512 //code:Np:profile
#define QUERY_ALLOCATE_RESPONSE 200

#define INIT_FILE "init.dae"
#define CONF_FILE "conf.dae"
#define INIT_SERV_FILE "init.serv"
#define CONF_SERV_FILE "conf.serv"

/**
        Prints manual and interpretation of results.

*/
void print_manual();


/**
        Computes the size of each package from the number of devices, interfaces, and samples that are sent in each packet.

*/
void calculate_packed_size();

#endif
