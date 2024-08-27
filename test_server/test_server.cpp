#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include "servidor_monitor.hpp"
#include "library.h"
#include "servidor_monitor.hpp"
#include <signal.h>
#include <wordexp.h>
#include <sysexits.h>
#include <thread>


int opt, port = -1, master_port = -1;
char *server = NULL, *server1 = NULL, *server2 = NULL, *port_s = NULL, *master_port_s = NULL;
int master_retransmitter=0, tmrflag = 0, recovery = 0;
char * elasticsearch_add = NULL;


void intHandler(int dummy) {
    std::cout << "Finishing tasks..." << std::endl;
    exit(0);
}


int main(int argc, char *argv[]) {
    char *program_name = argv[0];
    /*int opt, port = -1, client_port = -1 , flexmpi_port = -1, master_port = -1;
    char *server = NULL, *server1 = NULL, *server2 = NULL, *port_s = NULL, *client_port_s = NULL, *time_s = NULL, *flexmpi_port_s = NULL, *master_port_s = NULL;
    int master_retransmitter=0, tmr = 0, recovery = 0;
    //int mem_hot = 101, cache_hot = 101, net_hot = 101;
    char * elasticsearch_add = NULL;*/

    time_t time;
    int error = 0;

    signal(SIGINT, intHandler);

    if (argc != 1) {
        //std::cout << "Usage: ./test_server -p <daemon_port> -n 0 [1 -s <ip_master_server> -r <port_master>] -k <es_ip> -m <tmr mode> [-y <backup_es1 -u <backup_es2>] -b <recovery>" << std::endl;
        std::cout << "Usage: ./test_server [params configured in init.serv file]" << std::endl;
        return -1;
    }

    /*while ((opt = getopt(argc, argv, "ds:p:r:s:m:b:k:y:u:n:")) != -1) {
        switch (opt) {
            case 'd':
                break;
            case 'p':
                port_s = optarg;
                port = strtol(optarg, NULL, 10);
                printf("Port %s\n", port_s);
                break;
            /*case 'c':
                client_port_s = optarg;
                client_port = strtol(optarg, NULL, 10);
                printf(Port socket client %s\n", client_port_s);
                break;*
            case 't':
                time_s = optarg;
                time = strtol(optarg, NULL, 10);
                printf("Check available machines for each %s seconds.\n", time_s);
                break;
            case 'n':
                master_retransmitter = strtol(optarg,NULL,10);
                break;
            case 'f':
                flexmpi_port_s = optarg;
                flexmpi_port = strtol(optarg, NULL, 10);
                printf("Port socket flexmpi %s\n", flexmpi_port_s);
                break;*
            case 'm':
                tmr = strtol(optarg, NULL, 10);
                printf("TMR mode: %i\n", tmr);
                break;
            case 'k':
                elasticsearch_add = optarg;
                printf("ES address %s\n", elasticsearch_add);
                break;
            /*case 'q':
                mem_hot = strtol(optarg, NULL, 10);
                if (mem_hot <= 0) mem_hot = 75;
                break;
            case 'w':
                cache_hot = strtol(optarg, NULL, 10);
                if (cache_hot <= 0) cache_hot = 75;
                break;
            case 'e':
                net_hot = strtol(optarg, NULL, 10);
                if (net_hot <= 0) net_hot = 75;
                break;*
                // INTERMEDIATE NODEW
            case 'r':
                master_port_s = optarg;
                master_port = strtol(optarg, NULL, 10);
                printf("Port socket master %s\n", master_port_s);
                break;
            case 's':
                server = optarg;
                printf("Master Server %s\n", server);
                break;
            case 'y':
                server1 = optarg;
                printf("Backup Server 1 %s\n", server1);
                break;
            case 'u':
                server2 = optarg;
                printf("Backup Server 2 %s\n", server2);
                break;
            case 'b':
                recovery = strtol(optarg, NULL, 10);
                printf("Recovery mode: %i\n", recovery);
                break;
            case '?':
                if ((optopt == 'p'))
                    fprintf(stderr, "Option -%c requieres an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            default:
                return -1;
        }
    }*/
    std::ifstream initfile(INIT_SERV_FILE);
    std::string line;
    int param_n = 0;
    int sizet = 0;
    while (std::getline(initfile, line))
    {
        if (line.length() > 0 && line.at(0) != '#') {
            switch (param_n) {
                case 0: //port
                    port = strtol(line.c_str(), NULL, 10);
                    printf("Port: %i\n", port);
                    param_n++;
                    break;
                case 1: //ES addr
                    sizet = strlen(line.c_str());
                    elasticsearch_add = (char *) malloc(sizet);
                    strcpy(elasticsearch_add, line.c_str());
                    printf("ElasticSearch IP: %s\n", elasticsearch_add);
                    param_n++;
                    break;
                case 2: // server bakup 1
                    sizet = strlen(line.c_str());
                    server1 = (char *) malloc(sizet);
                    strcpy(server1, line.c_str());
                    if (strcmp(server1, "-1") == 0) server1 = NULL;
                    printf("Server backup 1: %s\n", server1);
                    param_n++;
                    break;
                case 3: // server backup 2
                    sizet = strlen(line.c_str());
                    server2 = (char *) malloc(sizet);
                    strcpy(server2, line.c_str());
                    if (strcmp(server2, "-1") == 0) server2 = NULL;
                    printf("Server backup 2: %s\n", server2);
                    param_n++;
                    break;
                case 4: // TMR mode
                    tmrflag = strtol(line.c_str(), NULL, 10);
                    printf("TMR mode: %i\n", tmrflag);
                    param_n++;
                    break;
                case 5: // recovery mode
                    recovery = strtol(line.c_str(), NULL, 10);
                    printf("Recovery mode: %i\n", recovery);
                    param_n++;
                    break;
                case 6: //master port
                    master_port = strtol(line.c_str(), NULL, 10);
                    printf("Port socket master %i\n", master_port);
                    param_n++;
                    break;
                case 7: // master server address
                    sizet = strlen(line.c_str());
                    server = (char *) malloc(sizet);
                    strcpy(server, line.c_str());
                    if (strcmp(server, "-1") == 0) server = NULL;
                    printf("Master server IP: %s\n", server);
                    param_n++;
                    break;
                case 8: // aggregator flag
                    master_retransmitter = strtol(line.c_str(), NULL, 10);
                    printf("Aggregator: %i\n", master_retransmitter);
                    param_n++;
                    break;
                case 9:
                    printf("More lines than expected in init.serv file.\n");
                    break;
                default:
                    printf("Error reading input params\n");
                    return -1;
            }
        }
    }


    if ((port != -1 && (port < 1024) || (port > 65535)) /*|| (client_port != -1 && (client_port < 1024) || (client_port > 65535))
        || (flexmpi_port != -1 && (flexmpi_port < 1024) || (flexmpi_port > 65535))*/) {
        fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535\n");
        exit(EX_USAGE);
    }

    setElasticSearchAdd(elasticsearch_add);

    /*RECOVERY BEFORE CONFIGUING SERVER*/
    if (recovery == 1){
        //setDefaultTimer(time);
        db_initialization();
        recoveryMode(elasticsearch_add);
    }



    //setHotspotsValueNotification(mem_hot, cache_hot, net_hot);
    setTMRvalue(tmrflag);

    if (server != NULL && master_port_s != NULL){
        //intermediate node
        error = init_server(port);
        if (error == -1) {
            exit(error);
        }

        error = initialize_master_socket(server, master_port);
        if (error == -1) {
            exit(error);
        }

        if (server1 != nullptr)
            initialize_backup1_socket(server1, master_port);
            if (error == -1) {
                exit(error);
            }
        if (server1 != nullptr)
            initialize_backup2_socket(server2, master_port);
            if (error == -1) {
                exit(error);
            }

        /*Infinite loop, manage termination signal*/
        error = manage_intermediate_server_udp();


    } else {
        //master server
        //setDefaultTimer(time);
        if(recovery != 1)
            error = db_initialization();
        if (error == -1) {
            exit(error);
        }

        error = init_server(port);
        if (error == -1) {
            exit(error);
        }

        /*error = init_client_server(client_port);
        if (error == -1) {
            exit(error);
        }*/

        /*Launch a process to get stored messages and manage them*/
        //If I do this, I will waste a thread.
        /*pthread_attr_t attr;
        pthread_t thid;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thid, &attr, reinterpret_cast<void *(*)(void *)>(processor), NULL);*/
        processor();

        /*error = init_flexmpi_server(flexmpi_port);
        if (error == -1) {
            exit(error);
        }*/

        /*Infinite loop, manage termination signal*/
        error = manage_server_udp(master_retransmitter);
    }


    return 0;
}
