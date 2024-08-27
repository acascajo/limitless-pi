//
// Created by Alberto on 9/09/19.
//

#include "connector.h"
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <iostream>
#include <strings.h>
#include <cstring>
#include <vector>
#include <algorithm>


/*CURL METHODS FOR INFLUX DB*/

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
char * configure_curl_now_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip) {
    // using this doc page https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
    // behavior options
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);


    // network options
    //curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://localhost:8086/ping"); an old test
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=now");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, )// work here
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, NULL);


    char cmd [100];
    sprintf(cmd, "%s cpu=%i,ram=%i,gpu=%i,io=%i,temp=%i,net=%i",ip,cpu,ram,gpu,io,temp,net);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    //if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, "cpu value0=0,value1=872323,value2=928323,value3=238233,value4=3982332,value5=209233,value6=8732632,value7=4342421,value8=091092744,value9=230944") != CURLE_OK)
    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

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
char * configure_curl_avg_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip) {
    // using this doc page https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
    // behavior options
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);


    // network options
    //curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://localhost:8086/ping"); an old test
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=avg");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, )// work here
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, NULL);


    char cmd [100];
    sprintf(cmd, "%s cpu=%i,ram=%i,gpu=%i,io=%i,temp=%i,net=%i",ip,cpu,ram,gpu,io,temp,net);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    //if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, "cpu value0=0,value1=872323,value2=928323,value3=238233,value4=3982332,value5=209233,value6=8732632,value7=4342421,value8=091092744,value9=230944") != CURLE_OK)
    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

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
char * configure_curl_conf_operation(CURL *curl_easy_handler, int ncpu, int ncores, int nram, int nio, int nnets, int ngpu, char * ip) {
    // using this doc page https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
    // behavior options
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);


    // network options
    //curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://localhost:8086/ping"); an old test
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=conf");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, )// work here
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, NULL);


    char cmd [100];
    sprintf(cmd, "%s ncpus=%i,ncores=%i,nram=%i,nio=%i,ngpu=%i,nnet=%i",ip,ncpu,ncores,nram,nio,ngpu,nnets);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);
    line[strlen(cmd)] = '\0';

    //if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, "cpu value0=0,value1=872323,value2=928323,value3=238233,value4=3982332,value5=209233,value6=8732632,value7=4342421,value8=091092744,value9=230944") != CURLE_OK)
    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

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
char * configure_curl_avg_cluster_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip) {
    // using this doc page https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
    // behavior options
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);


    // network options
    //curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://localhost:8086/ping"); an old test
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=avg_cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, )// work here
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, NULL);


    char cmd [100];
    sprintf(cmd, "cluster ip=\"%s\",cpu=%i,ram=%i,gpu=%i,io=%i,temp=%i,net=%i",ip,cpu,ram,gpu,io,temp,net);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    //if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, "cpu value0=0,value1=872323,value2=928323,value3=238233,value4=3982332,value5=209233,value6=8732632,value7=4342421,value8=091092744,value9=230944") != CURLE_OK)
    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

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
char * configure_curl_now_cluster_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip) {
    // using this doc page https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
    // behavior options
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);


    // network options
    //curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://localhost:8086/ping"); an old test
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=now_cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, )// work here
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, NULL);


    char cmd [100];
    sprintf(cmd, "cluster ip=\"%s\",cpu=%i,ram=%i,gpu=%i,io=%i,temp=%i,net=%i",ip,cpu,ram,gpu,io,temp,net);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    //if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, "cpu value0=0,value1=872323,value2=928323,value3=238233,value4=3982332,value5=209233,value6=8732632,value7=4342421,value8=091092744,value9=230944") != CURLE_OK)
    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

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
char * configure_curl_conf_cluster_operation(CURL *curl_easy_handler, int ncpus, int ncores, int nram, int ngpu, int nio, int nnets, char * ip) {
    // using this doc page https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
    // behavior options
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);

    // network options
    //curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://localhost:8086/ping"); an old test
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=conf_cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, )// work here
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, NULL);


    char cmd [100];
    sprintf(cmd, "cluster ip=\"%s\",ncpus=%i,ncores=%i,nram=%i,ngpus=%i,nio=%i,nnets=%i",ip,ncpus,ncores,nram,ngpu,nio,nnets);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    //if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, "cpu value0=0,value1=872323,value2=928323,value3=238233,value4=3982332,value5=209233,value6=8732632,value7=4342421,value8=091092744,value9=230944") != CURLE_OK)
    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

/**
 * Drop time-series in Nodejs database
 * @param curl_easy_handler
 * @return
 */
void * configure_curl_now_js_drop_series_operation(CURL *curl_easy_handler) {
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?db=now_js&q=DROP+SERIES+FROM+cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);

    curl_easy_perform(curl_easy_handler);
}

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
char * configure_curl_now_js_operation(CURL *curl_easy_handler, int cpu, int ram, int gpu, int io, int temp, int net, char * ip) {
    curl_easy_setopt(curl_easy_handler, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/write?db=now_js");
    curl_easy_setopt(curl_easy_handler, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_TRANSFER_ENCODING, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_REDIR_PROTOCOLS, 0L);
    curl_easy_setopt(curl_easy_handler, CURLOPT_DEFAULT_PROTOCOL, "http");
    curl_easy_setopt(curl_easy_handler, CURLOPT_FOLLOWLOCATION, 0L);

    char cmd [100];
    sprintf(cmd, "cluster ip=\"%s\",cpu=%i,ram=%i,gpu=%i,io=%i,temp=%i,net=%i",ip,cpu,ram,gpu,io,temp,net);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    if (curl_easy_setopt(curl_easy_handler, CURLOPT_POSTFIELDS, line) != CURLE_OK)
        return NULL;

    return line;
}

/**
 * To create DB to store data
 * @param curl_easy_handler
 * @return
 */
void * create_curl_database_operation(CURL *curl_easy_handler){
    CURLcode res;
    //struct curl_slist* headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    /*curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+now");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    //curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+now_cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+now_js");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    /*curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+avg");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    //curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+avg_cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    /*curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+conf");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    //curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=CREATE+DATABASE+conf_cluster");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);
    curl_global_cleanup();
}

/**
 * To destroy de stored data.
 * @param curl_easy_handler
 * @return
 */
void * drop_curl_database_operation(CURL *curl_easy_handler){
    CURLcode res;
    //struct curl_slist* headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=DROP+DATABASE+now");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=DROP+DATABASE+avg");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);

    //headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_handler = curl_easy_init();
    curl_easy_setopt(curl_easy_handler, CURLOPT_URL, "http://127.0.0.1:8086/query?q=DROP+DATABASE+conf");
    curl_easy_setopt(curl_easy_handler, CURLOPT_POST, 1L);
    //curl_easy_setopt(curl_easy_handler, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl_easy_handler);

    /*omitted controls*/

    //curl_slist_free_all(headers);
    curl_easy_cleanup(curl_easy_handler);
    curl_global_cleanup();
}


/*CURL METHODS FOR ELASTICSEARCH + KIBANA*/

void curl_es_now(char *ip_es, int cpu, int ram, int gpu, int io, int temp, int net, char * ip, char* time){
    CURLcode res;
    char cmd [200];
    sprintf(cmd, "{\"ip\": \"%s\", \"cpu\": %i, \"ram\": %i, \"gpu\": %i, \"io\": %i, \"temp\": %i, \"net\": %i, \"timestamp\": \"%s\" }",ip,cpu,ram,gpu,io,temp,net, time);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    char uri [100];
    sprintf(uri, "http://%s:9200/myindex/message ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);

    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    //TO SEND CULR OUTPUT TO NULL
    FILE *wfd = fopen("/dev/null", "w");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, wfd);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    // **************************
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-now ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    free(line2);
    free(hs);
}

void curl_es_bulk_now(char* ip_es, char* line){
    CURLcode res;
    CURL *curl;

    char uri [100];
    sprintf(uri, "http://%s:9200/_bulk ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);


    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-now ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    //free(line);
    free(line2);
    free(hs);
}

void curl_es_processes(char *ip_es, int cpu, int ram, char* command, char * ip, char* time){
    CURLcode res;
    CURL *curl;
    char cmd [200];
    sprintf(cmd, R"({"ip": "%s", "cpu": %i, "ram": %i, "command": "%s", "timestamp": "%s" })",ip,cpu,ram, command, time);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    char uri [100];
    sprintf(uri, "http://%s:9200/myindexproc/message ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);


    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-proc ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(hs);
    free(line);
    free(line2);
}

void curl_es_ib(char *ip_es, char * ip, char* time){
    CURLcode res;
    char cmd [450];
    //Execute syscall with perfquey
    /*# Port counters: DR path slid 0; dlid 0; 0 port 1 (CapMask: 0x1600)
PortSelect:......................1
CounterSelect:...................0x0000
SymbolErrorCounter:…………..0
LinkErrorRecoveryCounter:........0
LinkDownedCounter:...............0
PortRcvErrors:...................0
PortRcvRemotePhysicalErrors:.....0
PortRcvSwitchRelayErrors:........0
PortXmitDiscards:................0
PortXmitConstraintErrors:........0
PortRcvConstraintErrors:.........0
CounterSelect2:..................0x00
LocalLinkIntegrityErrors:........0
ExcessiveBufferOverrunErrors:..
VL15Dropped:.....................0
PortXmitData:....................864
PortRcvData:.....................1008
PortXmitPkts:....................12
PortRcvPkts:.....................14
PortXmitWait:....................0
     */
    sprintf(cmd, "{\"ip\": \"%s\", \"symbolErrorC\": %i, \"LinkErrorRecoveryC\": %i, \"LinkDownedC\": %i, \"PortRcvErrs\": %i, \"PortRcvRemotePhysicalErr\": %i, \"PortRcvSwitchRelayErr\": %i, \"PortXmitDiscard\": %i, \"PortXmitConstraintErr\": %i, \"PortRcvConstraintErr\": %i, \"net\": %i, \"LocalLinkIntegrityErr\": %i, \"ExcesiveBuffOverrunErr\": %i, \"VL15Dropped\": %i, \"PortXmitData\": %i, \"PortRcvData\": %i, \"PortXmitPkts\": %i, \"PortRcvPkts\": %i, \"PortXmitWait\": %i, \"timestamp\": \"%s\" }",ip,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, time);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    char uri [100];
    sprintf(uri, "http://%s:9200/IBindex/message ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);

    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-now ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    free(line2);
    free(hs);
}

void curl_es_app(CURL *curl, char *ip_es, char *ip_node, int appid, char * timestamp, double rtime, double ptime, double ctime, double mflops, char *counter1_name, double counter1, char *counter2_name, double counter2, double iotime, double size, double diter){
    CURLcode res;
    char cmd [300];
    sprintf(cmd, "{\"ip\": \"%s\", \"appid\": %i, \"timestamp\": \"%s\", \"rtime\": %f, \"ptime\": %f, \"ctime\": %f, \"mflops\": %f, \"counter1_name\": \"%s\", \"counter1\": %f, \"counter2_name\": \"%s\", \"counter2\": %f, \"iotimee\": %f , \"size\": %f , \"diter\": %f   }",ip_node,appid,timestamp,rtime,ptime,ctime,mflops,counter1_name, counter1, counter2_name, counter2, iotime, size, diter);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    char uri [100];
    sprintf(uri, "http://%s:9200/myindexapp/message ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-now ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    free(line2);
    free(hs);
}

void curl_es_conf(char *ip_es, int ncpus, int ncores, int nram, int ngpu, int nio, int nnets, char * ip, char* time){
    CURLcode res;
    CURL *curl;
    char cmd [180];
    sprintf(cmd, "{\"ip\": \"%s\", \"ncpus\": %i, \"ncores\": %i, \"nram\": %i, \"ngpus\": %i,\"nio\": %i, \"nnets\": %i, \"timestamp\": \"%s\" }",ip,ncpus,ncores,nram,ngpu,nio,nnets, time);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    char uri [100];
    sprintf(uri, "http://%s:9200/myindexconf/message ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-conf ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    free(line2);
    free(hs);
}

void curl_es_conf(char *ip_es, int ncpus, int ncores, int nram, int ngpu, int nio, int nnets, const char * ip, char* time){
    CURLcode res;
    CURL *curl;
    char cmd [180];
    sprintf(cmd, "{\"ip\": \"%s\", \"ncpus\": %i, \"ncores\": %i, \"nram\": %i, \"ngpus\": %i,\"nio\": %i, \"nnets\": %i, \"timestamp\": \"%s\" }",ip,ncpus,ncores,nram,ngpu,nio,nnets, time);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    char uri [100];
    sprintf(uri, "http://%s:9200/myindexconf/message ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    struct curl_slist *hs=NULL;
    hs = curl_slist_append(hs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-conf ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    free(line2);
    free(hs);
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::vector<std::vector<std::string>> curl_es_get_conf(char * ip){

    char cmd [100];
    sprintf(cmd, "http://%s:9200/myindexconf/_search/?size=100&pretty&sort=timestamp:desc", ip);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, line);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_perform(curl);
    //std::cout << readBuffer << std::endl;
    //curl_easy_cleanup(curl);

    //get data from request
    std::string delimiter = "\"_source\" : {\n          ";

    size_t pos = 0;
    std::string token;
    int iteration = 0;
    std::vector<std::string> parsed;
    while ((pos = readBuffer.find(delimiter)) != std::string::npos) {
        token = readBuffer.substr(0, pos);
        //std::cout << token << std::endl;
        readBuffer.erase(0, pos + delimiter.length());
        if (iteration != 0){
            parsed.push_back(token);
        }
        iteration++;
    }
    parsed.push_back(readBuffer);

    delimiter = "},\n";
    token = "";
    std::vector<std::string> parsed_aux;
    for(std::string s : parsed) {
        pos = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            s.erase(0, pos + delimiter.length());
            parsed_aux.push_back(token);
            break;
        }
    }

    parsed.clear();
    for(std::string s : parsed_aux)
        parsed.push_back(s);
    parsed_aux.clear();

    delimiter = "\n";
    token = "";
    std::vector<std::vector<std::string>> data;
    for(std::string s : parsed) {
        pos = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            s.erase(0, pos + delimiter.length());
            parsed_aux.push_back(token);
        }
        parsed_aux.push_back(s);
        data.push_back(parsed_aux);
        parsed_aux.clear();
    }

    parsed.clear();

    std::vector<std::vector<std::string>> final_data;
    for(std::vector<std::string> sv : data) {
        int element = 0; //0 =  ip, 1-6 = data, 7 = timestamp
        for(std::string s : sv){
            if (element == 0 || element == 7){
                delimiter = "\"";
                pos = 0;
                iteration = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    s.erase(0, pos + delimiter.length());
                    if (iteration == 3)
                        parsed_aux.push_back(token);
                    iteration++;
                }
                element++;
            } else {
                delimiter = ":";
                pos = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    s.erase(0, pos + delimiter.length());
                    break;
                }
                delimiter = ",";
                pos = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    s.erase(0, pos + delimiter.length());
                    parsed_aux.push_back(token);
                }
                element++;
            }
        }
        final_data.push_back(parsed_aux);
        parsed_aux.clear();
    }


    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    return final_data;

}

std::vector<std::vector<std::string>> curl_es_get_now(char * ip){

    char cmd [100];
    sprintf(cmd, "http://%s:9200/myindex/_search/?size=100&pretty&sort=timestamp:desc", ip);
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, line);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_perform(curl);
    //std::cout << readBuffer << std::endl;
    //curl_easy_cleanup(curl);

    //get data from request
    std::string delimiter = "\"_source\" : {\n          ";

    size_t pos = 0;
    std::string token;
    int iteration = 0;
    std::vector<std::string> parsed;
    while ((pos = readBuffer.find(delimiter)) != std::string::npos) {
        token = readBuffer.substr(0, pos);
        //std::cout << token << std::endl;
        readBuffer.erase(0, pos + delimiter.length());
        if (iteration != 0){
            parsed.push_back(token);
        }
        iteration++;
    }
    parsed.push_back(readBuffer);

    delimiter = "},\n";
    token = "";
    std::vector<std::string> parsed_aux;
    for(std::string s : parsed) {
        pos = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            s.erase(0, pos + delimiter.length());
            parsed_aux.push_back(token);
            break;
        }
    }

    parsed.clear();
    for(std::string s : parsed_aux)
        parsed.push_back(s);
    parsed_aux.clear();

    delimiter = "\n";
    token = "";
    std::vector<std::vector<std::string>> data;
    for(std::string s : parsed) {
        pos = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            s.erase(0, pos + delimiter.length());
            parsed_aux.push_back(token);
        }
        parsed_aux.push_back(s);
        data.push_back(parsed_aux);
        parsed_aux.clear();
    }

    parsed.clear();

    std::vector<std::vector<std::string>> final_data;
    for(std::vector<std::string> sv : data) {
        int element = 0; //0 =  ip, 1-6 = data, 7 = timestamp
        for(std::string s : sv){
            if (element == 0 || element == 7){
                delimiter = "\"";
                pos = 0;
                iteration = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    s.erase(0, pos + delimiter.length());
                    if (iteration == 3)
                        parsed_aux.push_back(token);
                    iteration++;
                }
                element++;
            } else {
                delimiter = ":";
                pos = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    s.erase(0, pos + delimiter.length());
                    break;
                }
                delimiter = ",";
                pos = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    s.erase(0, pos + delimiter.length());
                    parsed_aux.push_back(token);
                }
                element++;
            }
        }
        final_data.push_back(parsed_aux);
        parsed_aux.clear();
    }


    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(line);
    return final_data;

}

char* curl_es_get_allocation(char* es_ip, int np, int profile){
    std::vector<std::vector<std::string>> conf = curl_es_get_conf(es_ip);
    conf = getLastConfMachines(conf);
    std::vector<std::vector<std::string>> now = curl_es_get_now(es_ip);
    char * res =  (char*)malloc(20*conf.size()*sizeof(conf[0]));
    int res_index = 0;
    std::vector<std::string> ips_added;


    //hacer un match entre las ips que están vivas con las medidas actuales (para no procesar el conjunto completo)
    for(auto i : conf) {
        for (auto j : now) {
            if (std::find(ips_added.begin(), ips_added.end(), i[0]) == ips_added.end()) {
                std::vector<std::string>::iterator p = std::find(j.begin(), j.end(), i[0]);
                if (p != j.end()) { //finded
                    //p contains the info
                    char *res_const = (char *) malloc(25);
                    std::string ip = i[0];
                    ips_added.push_back(ip);
                    int numCores = strtol(i[2].c_str(), nullptr, 10);
                    int cpu_usage = strtol(p[1].c_str(), nullptr, 10);
                    int mem_usage = strtol(p[2].c_str(), nullptr, 10);

                    //add these data to the packet. Return IP:num_free_cores:...
                    sprintf(res_const, "%s:%d:%d:", ip.c_str(), numCores, cpu_usage);
                    strcpy(&res[res_index], res_const);
                    res_index += strlen(res_const);
                    free(res_const);
                }
            }
        }
    }

    return res;
}

/**
 * search the last snapshot of machines
 * @param list
 * @return list of up nodes in the last sNapshot
 */
std::vector<std::vector<std::string>> getLastConfMachines(std::vector<std::vector<std::string>> list){
    std::vector<std::vector<std::string>> ret;
    int fails = 0;

    char* sampleTime = (char *)malloc(list[0][7].length());
    strcpy(sampleTime, list[0][7].c_str());
    time_t reftime = getTime(sampleTime);
    free(sampleTime);
    for (auto mach : list){
        sampleTime = (char *)malloc(mach[7].length());
        strcpy(sampleTime, mach[7].c_str());
        time_t machTime = getTime(sampleTime);
        if(reftime == machTime)
            ret.push_back(mach);
        else
            fails++;

        if(fails > 3) //conf is ordered desc. by timestamp. if fails searching the reftime it means that we are out of the last configuration.
            break;
    }

    return ret;
}



/*size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

DUPLUCATED
std::string get_es_allconfig(char *ip_es){
    CURLcode res;

    char uri [100];
    sprintf(uri, "http://%s:9200/myindexconf/_search/?size=100&pretty&sort=timestamp:desc ", ip_es);
    char* line2 = (char*)malloc(strlen(uri)+1);
    strcpy(line2, uri);

    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, line2);

    std::string response_string;
    std::string header_string;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

    char* url;
    long response_code;
    double elapsed;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-now ElasticSearch\n");
    curl_easy_cleanup(curl);
    curl = NULL;

    free(line2);
    return response_string;
}
 */

void curl_create_index(CURL *curl){
    CURLcode res;
    char cmd [100];
    sprintf(cmd, "{\"mappings\":{\"_default_\":{\"_timestamp\" : {\"enabled\" : true,\"store\" : true }}}}");
    char* line = (char*)malloc(strlen(cmd));
    strcpy(line, cmd);

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:9200/myindex ");
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,line);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,strlen(line));
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error curl-conf ElasticSearch\n");
    curl_easy_cleanup(curl);
}



/*     time functions     */

/**
 * gets the time_t from a formatted string time.
 * @param charTime
 * @return time_t
 */
time_t getTime(char* charTime){
    int hh, mm, ss, dd, mth, yy;
    sscanf( charTime, "%d/%d/%d %d:%d:%d", &yy, &mth, &dd, &hh, &mm, &ss);

    struct tm when ;
    memset( &when, 0, sizeof( struct tm ) ) ; // *****
    when.tm_mday = dd;
    when.tm_mon = mth - 1 ; // ***** Jan == 0, Dec == 11
    when.tm_year = yy - 1900 ; // ***** years since 1900
    when.tm_hour = hh;
    when.tm_min = mm;
    when.tm_sec = ss;

    time_t converted = mktime(&when);
    //std::cout << converted << '\n' ;
    return converted;
}



/**
 * gets the current time in string format
 * @return
 */
char * getTimestamp(){
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%Y/%m/%d %T", timeinfo);
    //puts(buffer);
    char* t = (char *) malloc(19);
    strcpy(t, buffer);
    return t;
}


