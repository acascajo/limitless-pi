#include <strings.h>
#include <unistd.h>
#include <sysexits.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include "servidor_monitor.hpp"
#include "daemon_monitor.hpp"
#include "server_data.hpp"
#include "network_data.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include "common.hpp"
#include "library.h"
#include "connector.h"
#include <algorithm>


#define NUM_ACCEPTED_CON 10
//#define BUFFER_SIZE 512


using namespace std;
int debug = 0;


//int sanswer = 3; //size of the answer
pthread_mutex_t mut_thread;
pthread_cond_t cond_thread;

pthread_cond_t empty;
pthread_mutex_t proc_mut;
int busy = 0; //if 0 false, if 1 true

/*Socket for listening in server*/
int sd_listen=0;
int sd_client_listen=0;
struct sockaddr_in si_me, si_other, si_master, si_bk1, si_bk2;

/*Socket for flexmpi controller*/
int sd_flex_listen=0;
struct sockaddr_in si_me_flex, si_other_flex;

/*socket descriptor for master and backup servers*/
int socket_desc_master = 0;
int socket_desc_bk1 = 0;
int socket_desc_bk2 = 0;

//vector: store all messages and, after full or timeout, send all to master
//std::vector<unsigned char *> _queue;
std::vector<struct handle_args> _global;
std::vector<struct handle_args> _queue_messages;
//unsigned char * _queue;
//int _queue_index = 0;

/*variables for hot-spots*/
int hotspot_enable = 0;
int mem_hot  = 75;
int cache_hot = 75;
int net_hot = 75;
int busy_hot = 0;
std::mutex mutex_hot;

/*Fault tolerance and replication*/
int tmr = 0;

//elasticsearch ip and generic port
char * e_s = nullptr;
int updated_port = 5000;
int up_master_port = 0;
char * up_server1 = NULL;
char * up_server2 = NULL;
char * up_server = NULL;


/*struct handle_args{
	char client_IP[30];
	unsigned char * buffer;
	ssize_t size;
};*/


/*     SERVER RECOVERY MODE      */

/**
 * Ask to ElasticSearch DB for the information to restore the server
 */
void recoveryMode(char* elasticsearch_add){
    std::vector<std::vector<std::string>> configs = curl_es_get_conf(elasticsearch_add);

    //We need to know how many of the results compose the last snapshot (same timestamp)
    std::string last_ts = configs[0][7];
    int counter = 0;
    for (int i = 0; i < configs.size(); i++){
        if (configs[i][7] == last_ts)
            counter++;
        else
            break; // Two snapshots have different timestamp, so if the value is different, the last snapshot has been processed
    }


    for(int i = 0; i < counter; i++) {
        std::vector<std::string> sv = configs[i];
        Hw_conf hw_conf;
        hw_conf.ip_addr_s = sv[0];
        hw_conf.mem_total = std::stoi(sv[3]);
        hw_conf.modo_bitmap = 0;
        hw_conf.n_devices_io = std::stoi(sv[5]);
        hw_conf.n_cpu =  std::stoi(sv[1]);
        hw_conf.n_cores =  std::stoi(sv[2]);
        hw_conf.n_gpu =  std::stoi(sv[4]);
        hw_conf.n_interfaces =  std::stoi(sv[6]);

        recovery_insert_hw_conf(hw_conf);
        db_insert_CONF(sv[0], std::stoi(sv[1]), std::stoi(sv[2]), std::stoi(sv[3]), std::stoi(sv[4]), std::stoi(sv[5]), std::stoi(sv[6]));
    }
}



/*    COMMUNICATION THREAD FUNCTIONS     */


/**
 * Auxiliar methods, forward declarations
 * @param local_inf
 */
void manageRequestUDP(struct handle_args *local_inf){
	unsigned char type;
	int btype_read;
	char * tmp_buf=NULL; 
	char clientId[30];
	Hw_conf hw_conf;
	int error = 0;		

	//sprintf(clientId,"(%s)",local_inf->client_IP);
	#ifdef DAEMON_SERVER_DEBUG
	//	cout << clientId << " Waiting for information from connected client" << endl;
	#endif
	//recvn function either receives n bytes by retrying recv or detects error (-1) or close/shutdown of peer (0)
	//receive request type (a char) or detect closed socket

	/********* obtaining request type (1 byte) ***********/
	type = local_inf->buffer[BTYPE_POS];
	//printf("Codigo recibido: %c\n", type);

	if (type == 'v') {
		//printf("Flex - Query\n");
		local_inf->socket = sd_flex_listen;
		local_inf->client_addr = &si_other_flex;
		manage_query_packet(&(local_inf->buffer[0]), si_other_flex, sd_flex_listen);
	}else if(type == 0){ //Configuration request
		
		#ifdef DAEMON_SERVER_DEBUG
			//cout << "New configuration packet at " << clientId <<"." << endl;
		#endif
		/****** receiving request header information ***********/
		/*OBtain the hardware configuration from client*/

		obtain_conf_from_packet(&local_inf->buffer[BTYPE_POS+1], &hw_conf,local_inf->client_IP);
		
		error = insert_hw_conf(hw_conf);
		if(error == -1){
			cerr << "Error saving hardware configuration."<< endl;
		}

		/********************************************************/
		#ifdef DAEMON_SERVER_DEBUG
			//print_hw_conf(&hw_conf);
		#endif		
	} else if (type < MAX_SAMPLES){ //Monitoring message
		
		//cout << "Llamando a manage_monitoring_packet" << endl;
		manage_monitoring_packet (&(local_inf->buffer[BTYPE_POS]), local_inf->size, local_inf->client_IP);


	}else { //Client query
		//printf("client - Query\n");
		local_inf->socket = sd_client_listen;
		local_inf->client_addr = &si_other;
		manage_query_packet(&(local_inf->buffer[0]), si_other, sd_client_listen);
		/*#ifdef DAEMON_SERVER_DEBUG
        cerr << clientId << "Request type unknown: " << type << ", closing connection." << endl;
    #endif*/
	}


	//returns to accept another request
}

/**
 * Forward queries from FlexMPI controller to its manager
 * @param local_inf
 */
void manageRequestAllocate(struct handle_args *local_inf) {
	unsigned char type;
	char clientId[32];
	Hw_conf hw_conf;
	int error = 0;

	sprintf(clientId, "(%s)", local_inf->client_IP);
#ifdef DAEMON_SERVER_DEBUG
	//cout << clientId << " Waiting for information from connected client" << endl;
#endif

	local_inf->socket = sd_flex_listen;
	local_inf->client_addr = &si_other_flex;
    //sockaddr_in s;
    //memcpy(&s, local_inf->client_addr, sizeof(sockaddr_in));
	manage_allocate_packet(&(local_inf->buffer[0]), si_other_flex, sd_flex_listen);//s, local_inf->socket);
}

/**
 * Thread function. It manages packages from Daemons and Clients
 * @param global
 * @return
 */
void *run(void *global) {
	struct handle_args* global_inf;
	struct handle_args local_inf;
	
	//auto start = std::chrono::high_resolution_clock::now();

	global_inf = (struct handle_args * )global;

	//This mutext and the next things commented are for launches without worker threads (when we launched global threads)
	//pthread_mutex_lock(&mut_thread);
	/*Allocate temporal buffer with size of request + size of IP*/
	local_inf.buffer = (unsigned char *) calloc(global_inf->size+IP_SIZE, sizeof(unsigned char));
	/*Copy into local buffer*/
	memcpy(local_inf.client_IP, global_inf->client_IP, sizeof(local_inf.client_IP));
	local_inf.size = global_inf->size;
	memcpy(local_inf.buffer,global_inf->buffer, local_inf.size);
	busy=0;
	//pthread_cond_signal(&cond_thread);
	//pthread_mutex_unlock(&mut_thread);
	//Process request
	manageRequestUDP(&local_inf);
        
	/*auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Processig time: " << duration.count() << std::endl;*/

	free(local_inf.buffer);
	//pthread_exit(NULL);
}

/**
 * Thread function. It manages packages from FlexMPI Controller.
 * @param global
 * @return
 */
void *run_flex(void *global) {
	struct handle_args* global_inf;
	struct handle_args local_inf;

	global_inf = (struct handle_args * )global;
	pthread_mutex_lock(&mut_thread);
	/*Allocate temporal buffer with size of request + size of IP*/
	local_inf.buffer = (unsigned char *) calloc(global_inf->size+IP_SIZE, sizeof(unsigned char));
	/*Copy into local buffer*/
	memcpy(local_inf.client_IP, global_inf->client_IP, sizeof(local_inf.client_IP));
	local_inf.size = global_inf->size;
	memcpy(local_inf.buffer,global_inf->buffer, local_inf.size);
	busy=0;
	pthread_cond_signal(&cond_thread);
	pthread_mutex_unlock(&mut_thread);
	//Process request
	manageRequestAllocate(&local_inf);
	//cout << "Llamando al manage RequestUDP" << endl;
	free(local_inf.buffer);
	pthread_exit(NULL);
}

/**
* Initialize the server parameters, binding socket to available port.
* IN:
* @port	POrt in which the server will be binded
* RETURNS Socket descriptor or -1 in case of error.
*/
int init_server(int port) {
    int error = 0;
    updated_port = port;

    struct sockaddr_in server_addr;
    int val;

    /* Inicializar todo el servidor */
    //creating socket and setting its options
    sd_listen = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd_listen <= 0) {
        cerr << "An error ocurred while opening socket." << endl;
        error = -1;
        return error;
    }
    val = 1;
    setsockopt(sd_listen, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));

    //initializing sockaddr_in used to store server-side socket
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec = 250000;
    setsockopt(sd_listen, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));

    //binding
    error = bind(sd_listen, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (error == -1) {
        cerr << "Error binding port. Possibly already in use." << endl;
        error = -1;
        return error;
    }
    cout << "Init server " << inet_ntoa(server_addr.sin_addr) << ":" << port << endl;
#ifdef DAEMON_SERVER_DEBUG
    cout << "Init server: " << inet_ntoa(server_addr.sin_addr) << ", " << port << "." << endl;
#endif

    pthread_mutex_init(&proc_mut, NULL);
    pthread_cond_init(&empty, NULL);

    return error;
}

/**

	Initialize socket for communication.
	@param server String containing the Ip address to teh server towars the client will
	communicate.
	@param port_s String with the number of the port in which the server will be listening.
	@return Error in case the socket was not correctly created.
*/
int initialize_master_socket(char *serverm, int port){
	int error = 0;
    up_master_port = port;

    struct hostent *hp=NULL;

    //_queue = (unsigned char *)calloc(10 * MAX_PACKET_SIZE, sizeof(unsigned char));

	if ( (socket_desc_master=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		std::cerr << " Could not connect to master. " << std::endl;
		exit(0);
	}

	//timeout if packet lost or communation error
	/*struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec= 250000;
    setsockopt(socket_desc_client, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));*/

    hp = gethostbyname (serverm);
	bzero((char *)&si_master, sizeof(si_master));
    if(hp != NULL){
        memcpy(&(si_master.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
        return error;
    }

	//memset((char *) &si_master, 0, sizeof(si_master));
	si_master.sin_family = AF_INET;
	si_master.sin_port = htons(port);

	/*if (inet_aton(server , &si_master.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		//exit(1);
	}*/
    if(connect(socket_desc_master,(struct sockaddr *) &si_master, sizeof(si_master)) == -1){
        printf("Error connecting to the server...\n");
        error = -1;
    }


    return error;
}
int initialize_backup1_socket(char *serverm, int port){
    int error = 0;
    up_master_port = port;
    struct hostent *hp=NULL;

    //_queue = (unsigned char *)calloc(10 * MAX_PACKET_SIZE, sizeof(unsigned char));

    if ( (socket_desc_bk1=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        std::cerr << " Could not connect to master. " << std::endl;
        exit(0);
    }

    //timeout if packet lost or communation error
    /*struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec= 250000;
    setsockopt(socket_desc_client, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));*/

    hp = gethostbyname (serverm);
    bzero((char *)&si_bk1, sizeof(si_bk1));
    if(hp != NULL){
        memcpy(&(si_bk1.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
        return error;
    }

    //memset((char *) &si_master, 0, sizeof(si_master));
    si_bk1.sin_family = AF_INET;
    si_bk1.sin_port = htons(port);

    /*if (inet_aton(server , &si_master.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        //exit(1);
    }*/
    if(connect(socket_desc_bk1,(struct sockaddr *) &si_bk1, sizeof(si_bk1)) == -1){
        printf("Error connecting to the server...\n");
        error = -1;
    }


    return error;
}
int initialize_backup2_socket(char *serverm, int port){
    int error = 0;
    up_master_port = port;
    struct hostent *hp=NULL;

    //_queue = (unsigned char *)calloc(10 * MAX_PACKET_SIZE, sizeof(unsigned char));

    if ( (socket_desc_bk2=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        std::cerr << " Could not connect to master. " << std::endl;
        exit(0);
    }

    //timeout if packet lost or communation error
    /*struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec= 250000;
    setsockopt(socket_desc_client, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));*/

    hp = gethostbyname (serverm);
    bzero((char *)&si_bk2, sizeof(si_bk2));
    if(hp != NULL){
        memcpy(&(si_bk2.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
        return error;
    }

    //memset((char *) &si_master, 0, sizeof(si_master));
    si_bk2.sin_family = AF_INET;
    si_bk2.sin_port = htons(port);

    /*if (inet_aton(server , &si_master.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        //exit(1);
    }*/
    if(connect(socket_desc_bk2,(struct sockaddr *) &si_bk2, sizeof(si_bk2)) == -1){
        printf("Error connecting to the server...\n");
        error = -1;
    }


    return error;
}


/**
 * Update internal params from a file edited by user
 */
void updateConfParamsServ(){

    std::cout << "\n********** Updating parameters *************" << std::endl;
    std::ifstream initfile(CONF_SERV_FILE);
    std::string line;
    int param_n = 0;
    int sizet = 0;
    while (std::getline(initfile, line))
    {
        if (line.length() > 0 && line.at(0) != '#') {
            switch (param_n) {
                case 0: //port
                    updated_port = strtol(line.c_str(), NULL, 10);
                    printf("Port: %i\n", updated_port);
                    param_n++;
                    init_server(updated_port);
                    break;
                case 1: //ES addr
                    sizet = strlen(line.c_str());
                    free(e_s);
                    e_s = (char *) malloc(sizet);
                    strcpy(e_s, line.c_str());
                    printf("ElasticSearch IP: %s\n", e_s);
                    param_n++;
                    break;
                case 2: // server bakup 1
                    sizet = strlen(line.c_str());
                    up_server1 = (char *) malloc(sizet);
                    strcpy(up_server1, line.c_str());
                    if (strcmp(up_server1, "-1") == 0) up_server1 = NULL;
                    printf("Server backup 1: %s\n", up_server1);
                    param_n++;
                    break;
                case 3: // server backup 2
                    sizet = strlen(line.c_str());
                    up_server2 = (char *) malloc(sizet);
                    strcpy(up_server2, line.c_str());
                    if (strcmp(up_server2, "-1") == 0) up_server2 = NULL;
                    printf("Server backup 2: %s\n", up_server2);
                    param_n++;
                    break;
                case 4: // TMR mode
                    tmr = strtol(line.c_str(), NULL, 10);
                    printf("TMR mode: %i\n", tmr);
                    param_n++;
                    break;
                case 5: //master port
                    up_master_port = strtol(line.c_str(), NULL, 10);
                    printf("Port socket master %i\n", up_master_port);
                    param_n++;
                    break;
                case 6: // master server address
                    sizet = strlen(line.c_str());
                    up_server = (char *) malloc(sizet);
                    strcpy(up_server, line.c_str());
                    if (strcmp(up_server, "-1") == 0) up_server = NULL;
                    printf("Master server IP: %s\n", up_server);
                    param_n++;
                    break;
                case 7: {// aggregator flag
                    int master_ret = strtol(line.c_str(), NULL, 10);
                    printf("Aggregator: %i\n", master_ret);
                    param_n++;
                    }
                    break;
                case 8:
                    printf("More lines than expected in conf.serv file.\n");
                    break;
                default:
                    break;
            }
        }

    }

    initialize_backup1_socket(up_server1, up_master_port);
    initialize_backup2_socket(up_server2, up_master_port);
}


/*int manage_server_tcp(){
	int size=0;
	int sc=0;
	int error =0;
	struct  sockaddr_in client_addr;
	size = sizeof(client_addr);
	char client_ad[16];
	req_inf information_thread;
	pthread_attr_t attr;
	pthread_t thid;
	
	//setting maximum number of requests in queue to 5	
	error= listen(sd_listen,NUM_ACCEPTED_CON);

	if (error==-1){
		cerr << "Error listening on port." << endl;
		error = -1;
	}
	//creating thread attributes and setting them to "detached"
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_mutex_init(&mut_thread,NULL);
	pthread_cond_init(&cond_thread, NULL);
	
	while (1)
	{

#ifdef DAEMON_SERVER_DEBUG
		cout << "Waiting for new connection..." << endl;
#endif
		sc = accept(sd_listen, (struct sockaddr *) &client_addr, (socklen_t *) &size);

		//get ip address in a string
		sprintf(client_ad, inet_ntoa(client_addr.sin_addr),sizeof(client_ad));

#ifdef DAEMON_SERVER_DEBUG
		cout << "Accept " << client_ad << ":" << client_addr.sin_port << "." << endl;
#endif		

		//information to pass to the thread (socket descriptor, client address and port in a req_inf structure that stores this information)
		information_thread.socket = sc;
		strncpy((char *)&information_thread.address,client_ad,sizeof(client_ad));
		information_thread.port= client_addr.sin_port;

		pthread_create(&thid, &attr,run,&information_thread);
		
		//wait for child to make a local copy of request
		pthread_mutex_lock(&mut_thread);
		while (busy==1){
			pthread_cond_wait(&cond_thread,&mut_thread);
		}
		busy=1;
		pthread_mutex_unlock(&mut_thread);
		
	
	}

	return 0;
}
*/

/**
* Initialize the client server parameters, binding socket to available port.
* IN:
* @port	POrt in which the client server will be binded
* RETURNS Socket descriptor or -1 in case of error.
*/
int init_client_server(int port){
    int error = 0;

    //create a UDP socket
    if ((sd_client_listen=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        cerr << "An error ocurred while opening socket." << endl;
        error = -1;
        return error;
    }

    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec= 250000;
    setsockopt(sd_client_listen, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));

    //bind socket to port
    if( bind(sd_client_listen , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        cerr << "Error binding port. Possibly already in use." << endl;
        error = -1;
        return error;
    }

    cout << "Init client server " << inet_ntoa(si_me.sin_addr) << ":" << port << endl;

    return error;
}

/**
* Initialize the flexMPI server parameters, binding socket to available port.
* IN:
* @port	Port in which the flexmpi controller will be binded
* RETURNS Socket descriptor or -1 in case of error.
*/
int init_flexmpi_server(int port){
    int error = 0;

    //create a UDP socket
    if ((sd_flex_listen=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        cerr << "An error ocurred while opening socket." << endl;
        error = -1;
        return error;
    }

    // zero out the structure
    memset((char *) &si_me_flex, 0, sizeof(si_me_flex));

    si_me_flex.sin_family = AF_INET;
    si_me_flex.sin_port = htons(port);
    si_me_flex.sin_addr.s_addr = htonl(INADDR_ANY);

    struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec= 250000;
    setsockopt(sd_flex_listen, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));

    //bind socket to port
    if( bind(sd_flex_listen , (struct sockaddr*)&si_me_flex, sizeof(si_me_flex) ) == -1)
    {
        cerr << "Error binding port. Possibly already in use." << endl;
        error = -1;
        return error;
    }

    cout << "Init flexMPI server " << inet_ntoa(si_me.sin_addr) << ":" << port << endl;

    return error;
}

/**
 * Method to manage an UDP server. It contains one listener reserved to the Daemon requests.
 * @return socket descriptor or -1 if error.
 */
int manage_server_udp(int master_ret) {
	int sc = 0;
	struct sockaddr_in client_addr, flexmpi_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	char client_ad[16];
	char flex_ad[16];
	unsigned char *tmp_buffer;
	//struct handle_args *tmp;
	struct handle_args ha_thread;
	pthread_attr_t attr;
	pthread_t thid;

	//std::cout<< "Queue messages max-elements " << _queue_messages.max_size() << std::endl;

	tmp_buffer = (unsigned char *) calloc(MAX_PACKET_SIZE, sizeof(unsigned char));
	//creating thread attributes and setting them to "detached"
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_mutex_init(&mut_thread, NULL);
	pthread_cond_init(&cond_thread, NULL);

    long last_conf_update = -1;

	while (1) {

        /************************* CHECK IF THERE IS A CONFIGURATION UPDATE*************/

        struct stat filestat;
        if(stat(CONF_FILE, &filestat)==0) //si se ha creado
        {
            if (last_conf_update == -1) { //procesar directamente
                last_conf_update = filestat.st_mtime;
                updateConfParamsServ();
            }
            else //comparar primero si hay actualización
            {
                auto t = filestat.st_mtime;
                if (t > last_conf_update){
                    last_conf_update = filestat.st_mtime;
                    updateConfParamsServ();
                }
            }

        }

        /******************************************************************************/

		//This is the socket listener for DaeMon
#ifdef DAEMON_SERVER_DEBUG
		//cout << "Waiting for new request..." << endl;
#endif
		ssize_t size_packet = recvfrom(sd_listen, tmp_buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &client_addr, &client_addr_size);
		/*Receive packet of max length.*/
		if (size_packet == -1) {
			//The timeout implies that size_packet will be -1.
			//cerr << "Error receiving packet. " << endl;
		} else {
			/*Packet was succesfully obtained*/
			//get ip address in a string
			sprintf(client_ad, inet_ntoa(client_addr.sin_addr), sizeof(client_ad));

#ifdef DAEMON_SERVER_DEBUG
			//cout << "Received from " << client_ad << ":" << client_addr.sin_port << "." << endl;
#endif

			//information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
			ha_thread.buffer = (unsigned char *) tmp_buffer;
			memcpy(ha_thread.buffer, tmp_buffer, sizeof(tmp_buffer));
			strcpy(ha_thread.client_IP, client_ad);
			ha_thread.size = size_packet;
			ha_thread.socket = sd_listen;
			ha_thread.client_addr = &client_addr;

			if(master_ret == 1) { //Only when there are retransmitters
				std::string new_ip = "";
				for (int i = ha_thread.size - 4; i < ha_thread.size; i++) {
					new_ip = new_ip + std::to_string((int) ha_thread.buffer[i]);
					if (i != ha_thread.size - 1)
						new_ip = new_ip + ".";
				}
				strcpy(ha_thread.client_IP, new_ip.c_str());
			}

			//pthread_create(&thid, &attr, run, &ha_thread);
            _queue_messages.push_back(ha_thread);
			pthread_cond_signal(&empty);
			/*if(_queue_messages.size() > 5)
			    std::cout << "queue size 5\n";
			else if(_queue_messages.size() > 10)
                std::cout << "queue size 10\n";
			else if(_queue_messages.size() > 50)
                std::cout << "queue size 50\n";
            */

            //pthread_create(&thid, &attr, reinterpret_cast<void *(*)(void *)>(dispatcher), &ha_thread);


            //wait for child to make a local copy of request
			/*pthread_mutex_lock(&mut_thread);
			while (busy == 1) {
				pthread_cond_wait(&cond_thread, &mut_thread);
			}
			busy = 1;
			pthread_mutex_unlock(&mut_thread);*/
		}


		//This is the socket listener for the client
		//char buf[QUERY_PACKET_SIZE];
		/*char buf[QUERY_PACKET_SPECT];
		//char client_ad[16];
		socklen_t slen = sizeof(si_other);
		//try to receive some data, this is a blocking call
		size_packet = recvfrom(sd_client_listen, buf, QUERY_PACKET_SPECT, 0, (struct sockaddr *) &si_other, &slen);
		if (size_packet == -1) {
			//The timeout implies that size_packet will be -1.
			//cerr << "Error receiving packet. " << endl;
		} else {
			//get ip address in a string
			sprintf(client_ad, inet_ntoa(si_other.sin_addr), sizeof(si_other));

#ifdef DAEMON_SERVER_DEBUG
			//cout << "Received from " << client_ad << ":" << si_other.sin_port << "." << endl;
#endif

			//information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
			ha_thread.buffer = (unsigned char *) buf;
			strcpy(ha_thread.client_IP, client_ad);
			ha_thread.size = size_packet;
			ha_thread.socket = sd_client_listen;
			ha_thread.client_addr = &si_other;
			pthread_create(&thid, &attr, run, &ha_thread);

			//wait for child to make a local copy of request
			pthread_mutex_lock(&mut_thread);
			while (busy == 1) {
				pthread_cond_wait(&cond_thread, &mut_thread);
			}
			busy = 1;
			pthread_mutex_unlock(&mut_thread);
		}*/

		//This is the socket listener for flexmpi requests
		/*char flex_buf[QUERY_ALLOCATE_SIZE];
		socklen_t flen = sizeof(si_other_flex);
		//try to receive some data, this is a blocking call
		size_packet = recvfrom(sd_flex_listen, flex_buf, QUERY_ALLOCATE_SIZE, 0, (struct sockaddr *) &si_other_flex, &flen);

		if (size_packet == -1) {
			//The timeout implies that size_packet will be -1.
			//cerr << "Error receiving packet. " << endl;
		} else {
			//get ip address in a string
			sprintf(flex_ad, inet_ntoa(si_other_flex.sin_addr), sizeof(si_other_flex));

#ifdef DAEMON_SERVER_DEBUG
			//cout << "Received from " << flex_ad << ":" << si_other_flex.sin_port << "." << endl;
#endif
			//information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
			ha_thread.buffer = (unsigned char *) flex_buf;
			strcpy(ha_thread.client_IP, flex_ad);
			ha_thread.size = size_packet;
			ha_thread.socket = sd_flex_listen;
			ha_thread.client_addr = &si_other_flex;
			pthread_create(&thid, &attr, run_flex, &ha_thread);

			hotspot_enable = 1;

			//wait for child to make a local copy of request
			pthread_mutex_lock(&mut_thread);
			while (busy == 1) {
				pthread_cond_wait(&cond_thread, &mut_thread);
			}
			busy = 1;
			pthread_mutex_unlock(&mut_thread);
		}*/

	}

	return 0;
}

/**
 * Sets the TMR mode
 * @param val
 */
void setTMRvalue(int val){
    tmr = val;
}

/**
 * Set the elasticsearch address
 * @param add
 */
void setElasticSearchAdd(char * add){
    e_s = add;
}



/*   ELASTICSEARCH ANALYTICS ROLE FUNCTIONS      */


/**
 * Method to manage an UDP server for analytic role. It contains two listeners, one
 * reserved to the clarisse requests and other reserved to the Client requests.
 * @return socket descriptor or -1 if error.
 */
void manage_analytic_server_udp() {
    int sc = 0;
    struct sockaddr_in client_addr, flexmpi_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char client_ad[16];
    char flex_ad[16];
    unsigned char *tmp_buffer;
    //struct handle_args *tmp;
    struct handle_args ha_thread;
    pthread_attr_t attr;
    pthread_t thid;

    //std::cout<< "Queue messages max-elements " << _queue_messages.max_size() << std::endl;

    tmp_buffer = (unsigned char *) calloc(MAX_PACKET_SIZE, sizeof(unsigned char));
    //creating thread attributes and setting them to "detached"
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_init(&mut_thread, NULL);
    pthread_cond_init(&cond_thread, NULL);

    while (1) {

        //This is the socket listener for the client
        char buf[QUERY_PACKET_SPECT];
        //char client_ad[16];
        socklen_t slen = sizeof(si_other);
        //try to receive some data, this is a blocking call
        ssize_t size_packet = recvfrom(sd_client_listen, buf, QUERY_PACKET_SPECT, 0, (struct sockaddr *) &si_other, &slen);
        if (size_packet == -1) {
            //The timeout implies that size_packet will be -1.
            //cerr << "Error receiving packet. " << endl;
        } else {
            //get ip address in a string
            sprintf(client_ad, inet_ntoa(si_other.sin_addr), sizeof(si_other));

#ifdef DAEMON_SERVER_DEBUG
            //cout << "Received from " << client_ad << ":" << si_other.sin_port << "." << endl;
#endif

            //information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
            ha_thread.buffer = (unsigned char *) buf;
            strcpy(ha_thread.client_IP, client_ad);
            ha_thread.size = size_packet;
            ha_thread.socket = sd_client_listen;
            ha_thread.client_addr = &si_other;
            pthread_create(&thid, &attr, run, &ha_thread);

            //wait for child to make a local copy of request
            pthread_mutex_lock(&mut_thread);
            while (busy == 1) {
                pthread_cond_wait(&cond_thread, &mut_thread);
            }
            busy = 1;
            pthread_mutex_unlock(&mut_thread);
        }

        //This is the socket listener for flexmpi requests
        char flex_buf[QUERY_ALLOCATE_SIZE];
        socklen_t flen = sizeof(si_other_flex);
        //try to receive some data, this is a blocking call
        size_packet = recvfrom(sd_flex_listen, flex_buf, QUERY_ALLOCATE_SIZE, 0, (struct sockaddr *) &si_other_flex, &flen);

        if (size_packet == -1) {
            //The timeout implies that size_packet will be -1.
            //cerr << "Error receiving packet. " << endl;
        } else {
            //get ip address in a string
            sprintf(flex_ad, inet_ntoa(si_other_flex.sin_addr), sizeof(si_other_flex));

#ifdef DAEMON_SERVER_DEBUG
            //cout << "Received from " << flex_ad << ":" << si_other_flex.sin_port << "." << endl;
#endif
            //information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
            ha_thread.buffer = (unsigned char *) flex_buf;
            strcpy(ha_thread.client_IP, flex_ad);
            ha_thread.size = size_packet;
            ha_thread.socket = sd_flex_listen;
            ha_thread.client_addr = &si_other_flex;
            pthread_create(&thid, &attr, run_flex, &ha_thread);

            hotspot_enable = 1;

            //wait for child to make a local copy of request
            pthread_mutex_lock(&mut_thread);
            while (busy == 1) {
                pthread_cond_wait(&cond_thread, &mut_thread);
            }
            busy = 1;
            pthread_mutex_unlock(&mut_thread);
        }

    }
}

/**
 *
 * @return elasticsearch address
 */
char* getElasticSearchAdd(){
    return e_s;
}


/**
 * thread that checks every X seconds the current state and takes a snapshot of available machines (if there was a change during the last measuring time)
 * @param hbTime
 */
void analyticFunction(int hbTime){
    //Obtain the whole machines and store the IPs
    std::vector<std::string> nodesdown;
    std::vector<std::vector<std::string>> nodes_snapshot;
    std::vector<std::string> aux;
    std::vector<std::string> processed;
    std::vector<std::vector<std::string>> conf = curl_es_get_conf(getElasticSearchAdd());


    char * referenceTime = getTimestamp();
    time_t refTime = getTime(referenceTime);
    //generate the first ip mapping
    for (auto v : conf) {
        aux.push_back(v[0]);
        aux.push_back(referenceTime);
        if(std::find(nodes_snapshot.begin(), nodes_snapshot.end(), aux) == nodes_snapshot.end()) {//not found
            nodes_snapshot.push_back(aux);
            curl_es_conf(getElasticSearchAdd(), stoi(v[1].c_str()), stoi(v[2].c_str()), stoi(v[3].c_str()),
                         stoi(v[4].c_str()), stoi(v[5].c_str()), stoi(v[6].c_str()), v[0].c_str(), referenceTime);
        }
        aux.clear();

    }
    free(referenceTime);

    //since now, each node in conf has the same timestamp


    while(1){
        nodesdown.clear();
        processed.clear();

        std::vector<std::vector<std::string>> now = curl_es_get_now(getElasticSearchAdd());
        referenceTime = getTimestamp();
        refTime = getTime(referenceTime);

        for (auto v : now){
            //check if all nodes are alive
            char* sampleTime = (char *)malloc(v[7].length());
            strcpy(sampleTime, v[7].c_str());
            time_t t = getTime(sampleTime);
            double seconds = difftime(refTime, t);
            free(sampleTime);

            //processed ip. If any node has been already processed, there is no sense to process another row
            std::vector<std::string>::iterator it;
            it = std::find(processed.begin(), processed.end(), v[0]);
            if (it == processed.end()) {
                processed.push_back(v[0]);

                it = std::find(nodesdown.begin(), nodesdown.end(), v[0]);
                if (it == nodesdown.end()) {
                    if (seconds > hbTime) {
                        //NODE DOWN
                        nodesdown.push_back(v[0]); //store the ip
                        //break; //if any node is down, it makes no sense to process any more --> maybe, i should send the information.
                    } else
                        processed.push_back(v[0]);
                }
            }
        }

        //if any node is down, regenerate config in ES
        conf.clear();
        conf = curl_es_get_conf(getElasticSearchAdd());
        conf = getLastConfMachines(conf);

        if (nodesdown.size() > 0) {
            nodes_snapshot.clear();

            for (auto w : conf) {
                bool drop = false;
                for (auto q : nodesdown) {
                    if (q.compare(w[0]) == 0)
                        drop = true;
                }
                if (!drop) {
                    curl_es_conf(getElasticSearchAdd(), stoi(w[1].c_str()), stoi(w[2].c_str()), stoi(w[3].c_str()), stoi(w[4].c_str()), stoi(w[5].c_str()), stoi(w[6].c_str()), w[0].c_str(), referenceTime);
                    aux.push_back(w[0]);
                    aux.push_back(referenceTime);
                    nodes_snapshot.push_back(aux);
                    aux.clear();
                }
            }

        }

        //If there is a new (or more) node
        if ((processed.size()/2) > (nodes_snapshot.size() + nodesdown.size())) {
            referenceTime = getTimestamp();
            for (auto w : conf) {
                bool add = false;
                for (auto q : nodes_snapshot) {
                    if (q[0].compare(w[0]) == 0)
                        add = true;
                }
                if (!add) {
                    curl_es_conf(getElasticSearchAdd(), stoi(w[1].c_str()), stoi(w[2].c_str()), stoi(w[3].c_str()), stoi(w[4].c_str()), stoi(w[5].c_str()), stoi(w[6].c_str()), w[0].c_str(), referenceTime);
                    aux.push_back(w[0]);
                    aux.push_back(referenceTime);
                    nodes_snapshot.push_back(aux);
                    aux.clear();
                }
            }
        }
        free(referenceTime);
        sleep(5); //BETTER THE SAME TIME THAN DAEMON MONITORING

    }
}



/*    NODE AGGREGATOR FUNCTIONS    */


/**
 * Method to manage an UDP server. It contains two listeners, one
 * reserved to the Daemon requests and other reserved to the Client requests.
 * @return socket descriptor or -1 if error.
 */
int manage_intermediate_server_udp() {
    int sc = 0;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char client_ad[16];
    unsigned char *tmp_buffer;

    struct handle_args ha_thread;
    pthread_attr_t attr;
    pthread_t thid;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&mut_thread, NULL);
    pthread_cond_init(&cond_thread, NULL);

    tmp_buffer = (unsigned char *) calloc(MAX_PACKET_SIZE, sizeof(unsigned char));

    int last_conf_update = -1;
    while (1) {

        /************************* CHECK IF THERE IS A CONFIGURATION UPDATE*************/

        struct stat filestat;
        if(stat(CONF_FILE, &filestat)==0) //si se ha creado
        {
            if (last_conf_update == -1) { //procesar directamente
                last_conf_update = filestat.st_mtime;
                updateConfParamsServ();
            }
            else //comparar primero si hay actualización
            {
                auto t = filestat.st_mtime;
                if (t > last_conf_update){
                    last_conf_update = filestat.st_mtime;
                    updateConfParamsServ();
                }
            }

        }

        /******************************************************************************/


        //This is the socket listener for DaeMon
#ifdef DAEMON_SERVER_DEBUG
        //cout << "Waiting for new request..." << endl;
#endif
        ssize_t size_packet = recvfrom(sd_listen, tmp_buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &client_addr, &client_addr_size);
        /*Receive packet of max length.*/
        if (size_packet == -1) {
            //The timeout implies that size_packet will be -1.
            //cerr << "Error receiving packet. " << endl;
        } else {
            /*Packet was succesfully obtained*/
            //get ip address in a string
            sprintf(client_ad, inet_ntoa(client_addr.sin_addr), sizeof(client_ad));

#ifdef DAEMON_SERVER_DEBUG
            //cout << "Received from " << client_ad << ":" << client_addr.sin_port << "." << endl;
#endif
			//information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
			ha_thread.buffer = tmp_buffer;
			strcpy(ha_thread.client_IP, client_ad);
			ha_thread.size = size_packet;
			ha_thread.socket = sd_listen;
			ha_thread.client_addr = &client_addr;
			//pthread_create(&thid, &attr, reinterpret_cast<void *(*)(void *)>(retransmitter), &ha_thread);
			if (ha_thread.buffer[0] == 0) {
				/*for(int j = 0; j < ha_thread.size; j++)
					std::cout << (int) ha_thread.buffer[j] << " ";
				std::cout << std::endl;*/
				sendConfNow(ha_thread);//local_inf.buffer);
			} else {
				//_queue.push_back(local_inf.buffer);
				/*for(int j = 0; j < local_inf.size; j++)
                    std::cout << (int) local_inf.buffer[j] << " ";*/
				_global.push_back(ha_thread);
				/*memcpy(&_queue[_queue_index], local_inf.buffer, MAX_PACKET_SIZE);
                _queue_index += MAX_PACKET_SIZE;*/
		}

			if (_global.size() == 1){//_queue.size() == 5) {//_queue_index == (10 * MAX_PACKET_SIZE)){
				sendAllMessages();
			}

			//wait for child to make a local copy of request
			/*pthread_mutex_lock(&mut_thread);
			while (busy == 1) {
				pthread_cond_wait(&cond_thread, &mut_thread);
			}
			busy = 1;
			pthread_mutex_unlock(&mut_thread);*/

        }
    }

    return 0;
}

/**
 * Manage intermediate packets. NOT USED. Intermediate server do it directly in its code.
 * @param tmp_buffer
 */
/*void retransmitter(void * global) {
	struct handle_args* global_inf;
	struct handle_args local_inf;
	global_inf = (struct handle_args * )global;
	pthread_mutex_lock(&mut_thread);
	/*Allocate temporal buffer with size of request + size of IP*
	local_inf.buffer = (unsigned char *) calloc(global_inf->size+IP_SIZE+IP_SIZE, sizeof(unsigned char));
	/*Copy into local buffer*
	memcpy(local_inf.client_IP, global_inf->client_IP, sizeof(local_inf.client_IP));
	local_inf.size = global_inf->size;
	memcpy(local_inf.buffer,global_inf->buffer, local_inf.size);


    if (local_inf.buffer[0] == 0) {
        for(int j = 0; j < local_inf.size; j++)
            std::cout << (int) local_inf.buffer[j] << " ";
        std::cout << std::endl;
        sendConfNow(local_inf);//local_inf.buffer);
    } else {
        //_queue.push_back(local_inf.buffer);
        /*for(int j = 0; j < local_inf.size; j++)
            std::cout << (int) local_inf.buffer[j] << " ";*
        _global.push_back(local_inf);
		/*memcpy(&_queue[_queue_index], local_inf.buffer, MAX_PACKET_SIZE);
		_queue_index += MAX_PACKET_SIZE;*
    }

    if (_global.size() == 1){//_queue.size() == 5) {//_queue_index == (10 * MAX_PACKET_SIZE)){
        sendAllMessages();
    }

	busy=0;
	pthread_cond_signal(&cond_thread);
	pthread_mutex_unlock(&mut_thread);
	free(local_inf.buffer);
	pthread_exit(NULL);
}
*/

/**
 * Send config to master without wait.
 * @param mes
 */
void sendConfNow(struct handle_args ha){//unsigned char * mes){
    int slen = sizeof(si_master);
    //add ip to mes
    std::vector<std::string> ip = split(ha.client_IP, '.');
    for (int i = 0; i < ip.size(); i++) {
        ha.buffer[ha.size] = (unsigned char) stoi(ip[i]);
        ha.size++;
    }

    /*for(int j = 0; j < ha.size; j++)
		    std::cout << (int) ha.buffer[j] << " ";*/

    if (sendto(socket_desc_master, ha.buffer, /*MAX_PACKET_SIZE*/ha.size , 0 , (struct sockaddr *) &si_master, slen)==-1)
    {
        std::cerr << " Error sending package. " << std::endl;
        //exit(1);
    }

    //TMR with other two servers.
    if(socket_desc_bk1 != 0)
        if (sendto(socket_desc_bk1, ha.buffer, /*MAX_PACKET_SIZE*/ha.size , 0 , (struct sockaddr *) &si_bk1, slen)==-1)
        {
            std::cerr << " Error sending package. " << std::endl;
            //exit(1);
        }

    if(socket_desc_bk2 != 0)
        if (sendto(socket_desc_bk2, ha.buffer, /*MAX_PACKET_SIZE*/ha.size , 0 , (struct sockaddr *) &si_bk2, slen)==-1)
        {
            std::cerr << " Error sending package. " << std::endl;
            //exit(1);
        }
}

/**
 * Send all messages stored in _queue
 */
void sendAllMessages(){
    //printf("Sending queue to master\n");

    std::vector<struct handle_args> aux(_global);
    //for (auto i : _global)
    //    free(i.buffer);
    _global.clear();
    /*_queue = (unsigned char *)calloc(10 * MAX_PACKET_SIZE, sizeof(unsigned char)); //_queue.clear();
    _queue_index = 0;*/

    for(auto i : aux) {
        //send to master
        int slen = sizeof(si_other);
        std::vector<std::string> ip = split(i.client_IP, '.');
        for (int j = 0; j < ip.size(); j++) {
            i.buffer[i.size] = (unsigned char) stoi(ip[j]);
            i.size++;
        }
        /*for(int j = 0; j < i.size; j++)
            std::cout << (int) i.buffer[j] << " ";*/

        // 3 tries to connect on each server
        for (int tmrpos = 0; tmrpos < 9; tmrpos++) {
            if (tmrpos < 3) {
                if (sendto(socket_desc_master, i.buffer, /*MAX_PACKET_SIZE*/i.size, 0, (struct sockaddr *) &si_master,
                           slen) == -1) {
                    std::cerr << " Error sending package. " << std::endl;
                    //exit(1);
                }
                else
                    if(tmr != 1)
                        break;
            } else if (tmrpos >= 3 && tmrpos <6) {
                if (sendto(socket_desc_bk1, i.buffer, /*MAX_PACKET_SIZE*/i.size, 0, (struct sockaddr *) &si_bk1,
                           slen) == -1) {
                    std::cerr << " Error sending package. " << std::endl;
                    //exit(1);
                } else
                    if(tmr != 1)
                        break;
            } else {
                if (sendto(socket_desc_bk2, i.buffer, /*MAX_PACKET_SIZE*/i.size, 0, (struct sockaddr *) &si_bk2,
                           slen) == -1) {
                    std::cerr << " Error sending package. " << std::endl;
                    //exit(1);
                } else
                    if(tmr != 1)
                        break;
            }
            //free(i.buffer);
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //prevents saturation in master (temporally)

        }
    }
    aux.clear();
}



/*   QUEUE MESSAGES FUNCTIONS    */

/**
 * To get an element from the front of the queue
 * @param vec
 */
void pop_front(std::vector<struct handle_args>& vec)
{
    vec.erase(vec.begin());
}

/**
 * Each processor thread gets data from the buffer and process it.
 */
void worker_function(){
    //get the mutex to get data from buffer. If it is empty, we have to wait.
    //int counter = 0;
    while(1) {
        pthread_mutex_lock(&proc_mut);
        while (queue_empty() == 1) {
            pthread_cond_wait(&empty, &proc_mut);
        }

        //if (_queue_messages.size() > 0) {
            //get a packet from the buffer
            auto i = _queue_messages[0];
            _queue_messages.erase(_queue_messages.begin());
            //process it
            //pthread_create(&thid, &attr, run, &i);

            //free queue mutex
            pthread_mutex_unlock(&proc_mut);
            run(&i);

            //wait for child to make a local copy of request
            /*pthread_mutex_lock(&mut_thread);
            while (busy == 1) {
                pthread_cond_wait(&cond_thread, &mut_thread);
            }
            busy = 1; // maybe it could be deleted*/
            //pthread_mutex_unlock(&mut_thread);

        //
        // }

        //std::cout << "[" << std::this_thread::get_id() << "] " << "Processed " << counter << "operations" << std::endl;
        //counter++;
    }
}

/**
 * Get messages from queue and process them.
 * V2: create only de max number of threads, taking into account that server uses 4 by default.
 */
void * processor() {
    int nthreads = 1;
    int max_concurrency = std::thread::hardware_concurrency(); //gets the max concurrency.
    std::cout << "Max concurrency detected: " << max_concurrency << std::endl;
    if (max_concurrency > 2) nthreads = max_concurrency -2;

    //V2: create threads that automatically process the information in the buffer.
    std::vector<std::thread> workers(nthreads);
    for (int i = 0; i < nthreads; i++){
        std::thread th(worker_function);
        th.detach();
    }



    /* V1
    while(1) {
        pthread_mutex_lock(&proc_mut);
        while (queue_empty() == 1) {
            pthread_cond_wait(&empty, &proc_mut);
        }

        if (_queue_messages.size() > 0) {
            if (nthreads > 0) {
                pthread_mutex_lock(&mut_thread);
                nthreads--;
                pthread_mutex_unlock(&mut_thread);

                pthread_attr_t attr;
                pthread_t thid;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                auto i = _queue_messages[0];
                _queue_messages.erase(_queue_messages.begin());
                pthread_create(&thid, &attr, run, &i);

                //wait for child to make a local copy of request
                pthread_mutex_lock(&mut_thread);
                while (busy == 1) {
                    pthread_cond_wait(&cond_thread, &mut_thread);
                }
                busy = 1; // maybe it could be deleted
                nthreads++;
                pthread_mutex_unlock(&mut_thread);
            }
        }
		pthread_mutex_unlock(&proc_mut);
    }*/

    /*for (auto i : _queue_messages){
        pthread_create(&thid, &attr, run, &i);
    }

    //wait for child to make a local copy of request
    pthread_mutex_lock(&mut_thread);
    while (busy == 1) {
        pthread_cond_wait(&cond_thread, &mut_thread);
    }
    busy = 1;
    pthread_mutex_unlock(&mut_thread);*/

}

/**
 * Return 0 if there are elements in queue.
 * @return
 */
int queue_empty(){
    return (_queue_messages.size() > 0) ? 0 : 1;
}

/**
 * Store messages in _queue_mesasages. Other thread get those messages
 * NOT USED: server do it directly in manage_server
 * @param global
 */
/*void *dispatcher(struct handle_args *global){//void * global) {
    struct handle_args* global_inf;
    struct handle_args local_inf;
    global_inf = (struct handle_args * )global;
    pthread_mutex_lock(&mut_thread);
    /*Allocate temporal buffer with size of request + size of IP*
    local_inf.buffer = (unsigned char *) calloc(global_inf->size+IP_SIZE+IP_SIZE, sizeof(unsigned char));
    /*Copy into local buffer*
    memcpy(local_inf.client_IP, global_inf->client_IP, sizeof(local_inf.client_IP));
    local_inf.size = global_inf->size;
    memcpy(local_inf.buffer,global_inf->buffer, local_inf.size);

    _queue_messages.push_back(local_inf);

    processor();

    busy=0;
    pthread_cond_signal(&cond_thread);
    pthread_mutex_unlock(&mut_thread);
    free(local_inf.buffer);
    pthread_exit(NULL);
}
*/



/*   HOTSPOTS FUNCTIONS   */

/**
 * If in any sample, one of the measures are grater than these, a hot-spot notification will send.
 * @param mem
 * @param cache
 * @param net
 */
void setHotspotsValueNotification(int mem, int cache, int net){
	mem_hot = mem;
	cache_hot = cache;
	net_hot = net;
}

/**
 * Get value hot-spot range to mem usage.
 * @return
 */
int getMemHotValue(){
	return mem_hot;
}

/**
 * Get value hot-spot range to cache usage.
 * @return
 */
int getCacheHotValue(){
	return cache_hot;
}

/**
 * Get value hot-spot range to network usage.
 * @return
 */
int getNetHotValue(){
	return net_hot;
}

/**
 * If  server detects hotspot (cpu, mem, io), send this information to flex_client.
 * @param ip
 * @param profile
 */
void hotspotNotification(std::string ip, int profile, char* mes){
    if (hotspot_enable != 0) {
        if (mutex_hot.try_lock()) {
            if (busy_hot == 0) {
                busy_hot = 1;
                auto hostname = (char *) calloc(NI_MAXHOST, sizeof(char));
                int err = obtainHostNameByIP(ip.c_str(), hostname);
                if (err == 0) {
                    auto out = (char *) calloc(strlen(hostname) + 3 + strlen(mes/*.c_str()*/),
                                               sizeof(char)); //ip + : + type + : + string
                    int index = 0;
                    strcpy(&out[index], hostname);
                    index += strlen(hostname);
                    out[index] = ':';
                    index++;
                    std::string aux = (profile == CPU_HOT) ? std::to_string(CPU_HOT) :
                                      (profile == MEM_HOT) ? std::to_string(MEM_HOT) :
                                      (profile == IO_HOT) ? std::to_string(IO_HOT) :
                                      (profile == NET_HOT) ? std::to_string(NET_HOT) : std::to_string(CACHE_HOT);
                    strcpy(&out[index], aux.c_str());
                    index++;
                    out[index] = ':';
                    index++;
                    strcpy(&out[index], mes);//.c_str());
                    index += strlen(mes);//.c_str());

                    size_t slen = sizeof(si_other);
                    ssize_t res = sendto(sd_flex_listen, out, strlen(out), 0, (struct sockaddr *) &si_other_flex, slen);
                    if (res < 0)
                        printf("Error sending packake\n");

                    //to log
                    ofstream myfile;
                    myfile.open ("/tmp/log_monitor/log_v2.xt", ios::out | ios::app);
                    myfile << "Hot-spot sended: " << hostname << ":" << mes << endl;
                    myfile.close();


                    free(out);
                } else {
                    auto out = (char *) calloc(strlen(ip.c_str()) + 3 + strlen(mes/*.c_str()*/),
                                               sizeof(char)); //ip + : + type + : + string
                    int index = 0;
                    strcpy(&out[index], ip.c_str());
                    index += strlen(ip.c_str());
                    out[index] = ':';
                    index++;
                    std::string aux = (profile == CPU_HOT) ? std::to_string(CPU_HOT) :
                                      (profile == MEM_HOT) ? std::to_string(MEM_HOT) :
                                      (profile == IO_HOT) ? std::to_string(IO_HOT) :
                                      (profile == NET_HOT) ? std::to_string(NET_HOT) : std::to_string(CACHE_HOT);
                    strcpy(&out[index], aux.c_str());
                    index++;
                    out[index] = ':';
                    index++;
                    strcpy(&out[index], mes);//.c_str());
                    index += strlen(mes);//.c_str());

                    size_t slen = sizeof(si_other);
                    ssize_t res = sendto(sd_flex_listen, out, strlen(out), 0, (struct sockaddr *) &si_other_flex, slen);
                    if (res < 0)
                        printf("Error sending packake\n");

                    //to log
                    ofstream myfile;
                    myfile.open ("/tmp/log_monitor/log_v2.xt", ios::out | ios::app);
                    myfile << "Hot-spot sended: " << ip << ":" << mes << endl;
                    myfile.close();

                    free(out);
                }
                free(hostname);
                busy_hot = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            mutex_hot.unlock();
        }
        //to log
        ofstream myfile;
        myfile.open ("/tmp/log_monitor/log_v2.xt", ios::out | ios::app);
        myfile << "Hot-spot NOT-sended (socket busy): " << ip << ":" << mes << endl;
        myfile.close();
    }
}


/*int main (int argc, char *argv[])
{
	int port, opt;

	while((opt = getopt(argc, argv, "dp:")) != -1)
	{
		switch (opt)
		{
			case 'p':
				port = strtol(optarg, NULL, 10);
				break;
			default:
				printf("Usage: ./server [-d] -p <port>\n");
				exit(EX_USAGE);
		}
	}

	if((port < 1024) || (port > 65535))
	{
		fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535\n");
		printf("Usage: ./server [-d] -p <port>\n");
		exit(EX_USAGE);
	}
	debug = 0
	fprintf(stderr, "PORT %d\n", port);
	if(debug)
		fprintf(stderr, "DEBUG ON\n");
	else
		fprintf(stderr, "DEBUG OFF\n");

	struct sockaddr_in server_addr, client_addr;
	int sd , sc;
	int val,size;

	//creating socket and setting its options
	//sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	val = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));

	//initializing sockaddr_in used to store server-side socket
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	//binding
	bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	printf("Init server %s:%i \n",inet_ntoa(server_addr.sin_addr),port);



	//setting maximum number of requests in queue to 5
	//listen(sd,5);


	size = sizeof(client_addr);
	char client_ad[16];
	char muestra[2];
	req_inf information_thread;
	pthread_attr_t attr;
	pthread_t thid;

	//creating thread attributes and setting them to "detached"
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_mutex_init(&mut_thread,NULL);
	pthread_cond_init(&cond_thread, NULL);

	socklen_t clilen = sizeof(client_addr);

	while (1)
	{
		printf("Waiting for new connection...\n");
		//sc = accept(sd, (struct sockaddr *) &client_addr, (socklen_t *) &size);

		recvfrom(sd, (unsigned char *) muestra, 2, 0, (struct sockaddr *)&client_addr, &clilen);
		printf("-%c-\n",muestra[0]);

		//get ip address in a string
		sprintf(client_ad, inet_ntoa(client_addr.sin_addr),sizeof(client_ad));
		printf("Accept %s:%i\n",client_ad,client_addr.sin_port);


		//information to pass to the thread (socket descriptor, client address and port in a req_inf structure that stores this information)
		information_thread.socket = sc;
		strncpy((char *)&information_thread.address,client_ad,sizeof(client_ad));
		information_thread.port= client_addr.sin_port;

		pthread_create(&thid, &attr,run,&information_thread);

		//wait for child to make a local copy of request
		pthread_mutex_lock(&mut_thread);
		while (busy==1){
			pthread_cond_wait(&cond_thread,&mut_thread);
		}
		busy=1;
		pthread_mutex_unlock(&mut_thread);


	}
	exit(0);
}*/








