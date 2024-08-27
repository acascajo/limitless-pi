//
// Created by Alberto on 9/09/19.
//


#include <curl/curl.h>
#include <vector>
#include <string>

#ifndef TEST_CLIENT_API_CONNECTOR_H
#define TEST_CLIENT_API_CONNECTOR_H


/**
 * Stores the current measures for each compute-node. Primary-key is IP
 * @param curl_easy_handler
 * @param cpu
 * @param ram
 * @param gpu
 * @param io
 * @param temp
 * @param net
 * @param ip
 * @return
 */
char * configure_curl_now_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip);

/**
 * Stores the avg(measures) for each compute-node.
 * @param curl_easy_handler
 * @param cpu
 * @param ram
 * @param gpu
 * @param io
 * @param temp
 * @param net
 * @param ip
 * @return
 */
char * configure_curl_avg_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip);

/**
 * Stores the configuration for each compute-node. Primary-key is IP
 * @param curl_easy_handler
 * @param ncpu
 * @param ncores
 * @param nram
 * @param nio
 * @param nnets
 * @param ngpu
 * @param ip
 * @return
 */
char * configure_curl_conf_operation(CURL *curl_easy_handler, int ncpu, int ncores, int nram, int nio, int nnets, int ngpu, char * ip);

/**
 * Stores the avg(measures) for each compute-node. ID is cluster (not ip)
 * @param curl_easy_handler
 * @param cpu
 * @param ram
 * @param gpu
 * @param io
 * @param temp
 * @param net
 * @param ip
 * @return
 */
char * configure_curl_avg_cluster_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip);

/**
 * Stores the current measures for each compute-node. Primary-key is cluster
 * @param curl_easy_handler
 * @param cpu
 * @param ram
 * @param gpu
 * @param io
 * @param temp
 * @param net
 * @param ip
 * @return
 */
char * configure_curl_now_cluster_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip);

/**
 * Stores the configuration for each compute-node. Primary-key is cluster
 * @param curl_easy_handler
 * @param ncpus
 * @param ncores
 * @param nram
 * @param ngpu
 * @param nio
 * @param nnets
 * @param ip
 * @return
 */
char * configure_curl_conf_cluster_operation(CURL *curl_easy_handler, int ncpus, int ncores, int nram, int ngpu, int nio, int nnets, char * ip);

/**
 * Drop time-series in Nodejs database
 * @param curl_easy_handler
 * @return
 */
void * configure_curl_now_js_drop_series_operation(CURL *curl_easy_handler);

/**
 * Same than now_cluster but for be accessed by NodeJs
 * @param curl_easy_handler
 * @param cpu
 * @param ram
 * @param gpu
 * @param io
 * @param temp
 * @param net
 * @param ip
 * @return
 */
char * configure_curl_now_js_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip);

/**
 * To create DB to store data
 * @param curl_easy_handler
 * @return
 */
void * create_curl_database_operation(CURL *curl_easy_handler);

/**
 * To destroy de stored data.
 * @param curl_easy_handler
 * @return
 */
void * drop_curl_database_operation(CURL *curl_easy_handler);


/*CURL METHODS FOR ELASTICSEARCH + KIBANA*/

void curl_es_now(char *ip_es, int cpu, int ram, int gpu, int io, int temp, int net, char * ip, char* time);

void curl_es_bulk_now(char* ip_es, char *line);

void curl_es_processes(char *ip_es, int cpu, int ram, char* command, char * ip, char* time);

void curl_es_ib(char *ip_es, char * ip, char* time);

void curl_es_app(CURL *curl, char *ip_es, char *ip_node, int appid, char * timestamp, double rtime, double ptime, double ctime, double mflops, char *counter1_name, double counter1, char *counter2_name, double counter2, double iotime, double size, double diter);

void curl_es_conf(char *ip_es, int ncpus, int ncores, int nram, int ngpu, int nio, int nnets, char * ip, char* time);

void curl_es_conf(char *ip_es, int ncpus, int ncores, int nram, int ngpu, int nio, int nnets, const char * ip, char* time);

std::vector<std::vector<std::string>> curl_es_get_conf(char * ip);

std::vector<std::vector<std::string>> curl_es_get_now(char * ip);

char* curl_es_get_allocation(char* es_ip, int np, int profile);

void curl_create_index(CURL *curl);

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp);

/*size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data);

std::string get_es_allconfig(char *ip_es);*/


std::vector<std::vector<std::string>> getLastConfMachines(std::vector<std::vector<std::string>> list);
time_t getTime(char* charTime);
char * getTimestamp();


#endif //TEST_CLIENT_API_CONNECTOR_H


