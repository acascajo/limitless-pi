#include "library.h"
#include <iostream>
#include <cstring>
#include <map>
#include <thread>
#include <pthread.h>
#include <unistd.h>
#include <network_data.hpp>
#include <netinet/in.h>

std::vector<std::string> _initNodes;
sqlite3 * db_global;

const char * _db_name = ":memory:";//":memory:";
const int _n_items = 127; /*This is the max number of samples that one node can send to the master.*/
std::map<std::string,int> _nodeTime;
std::map<std::string,int> _lastnodeTime;
std::map<std::string,int> _timer;
std::string _option = "all";
pthread_attr_t attr;
pthread_t thid;
time_t _time;

/* set a critical section to protect this variables to a race conditions*/
unsigned char * local_buffer;
char* _buffer;
int index_cb;

/**
 * This function opens database.
 *
 * @return 0 done, -1 error.
 */
int db_open(){
    int rc;

    /* Open database */
    rc = sqlite3_open(_db_name, &db_global);

    if( rc ) {
        fprintf(stderr, "Can't open database\n");
        return(-1);
    } else {
        //std::cout << "Opened database successfully" << std::endl;
    }
    return(0);
}

/**
 * This function closes database.
 * @return 0 if db closed. -1 if db is busy.
 */
int db_close() {
    int rc = sqlite3_close(db_global);
    if( rc ) {
        fprintf(stderr, "Can't close database\n");
        return(-1);
    } else {
        std::cout << "Database closed" << std::endl;
    }
    return(0);
}

/**
 * Create DB, and tables to store all data.
 * @param maxRegs Max samples per node that are allowed to store.
 * @param nodes List of node IP.
 * @return 0 if all tables are created successfuly or -1 if not.
 */
//int db_initialization(std::list<std::string> nodes){
int db_initialization(){
    char *zErrMsg = nullptr;
    int rc;
    const char *sql;
    local_buffer = (unsigned char *)malloc(SIZE_LOCAL_BUFFER * sizeof(unsigned char));
    _buffer = (char *)malloc(SIZE_LOCAL_BUFFER * sizeof(char));
    index_cb = 0; //to manage callback series


    /*Init indexes : it can't be here when merge.*/
    /*_initNodes = nodes;
    indexInit(std::move(nodes));*/

    /* Open database */
    rc = db_open();//sqlite3_open(_db_name, &db_global);
    if( rc != 0 )
        return(-1);

    /* Create SQL statement */
    /*idnode, number of cpus, number in gb of ram, 0: there is a gpu, % of cpu in use, % of ram in use*/
    sql = "CREATE TABLE COMPUTEROW("\
                "IP             TEXT                     NOT NULL,"\
                "TIME           INT                     NOT NULL,"\
                "IO_USAGE       INT                     NOT NULL,"\
                "CPU_USAGE      INT                     NOT NULL,"\
                "RAM_USAGE      INT                     NOT NULL,"\
                "GPU_USAGE      INT                     NOT NULL,"\
                "TEMP           INT                     NOT NULL,"\
                "NET            INT                     NOT NULL,"\
                "PRIMARY KEY (IP, TIME));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create main table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    /*idnode, number of cpus, number in gb of ram, 0: there is a gpu, % of cpu in use, % of ram in use*/
    sql = "CREATE TABLE BITMAP("\
                "IP             TEXT                     NOT NULL,"\
                "TIME           INT                     NOT NULL,"\
                "IO_USAGE       INT                     NOT NULL,"\
                "CPU_USAGE      INT                     NOT NULL,"\
                "RAM_USAGE      INT                     NOT NULL,"\
                "GPU_USAGE      INT                     NOT NULL,"\
                "PRIMARY KEY (IP, TIME));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create bitmap table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    /* This information is repeated, but it simplest to manage the Config. query. TODO: update select configuration and test the performance */
    sql = "CREATE TABLE CONFIGURATION("\
                "IP             TEXT                    NOT NULL,"\
                "NCPUS          INT                     NOT NULL,"\
                "NCORES         INT                     NOT NULL,"\
                "NRAM           INT                     NOT NULL,"\
                "NGPUS          INT                     NOT NULL,"\
                "NIO            INT                     NOT NULL,"\
                "NNETS          INT                     NOT NULL,"\
                "PRIMARY KEY (IP));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create config. table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    sql = "CREATE TABLE SUMMARY("\
                "IP             TEXT     PRIMARY KEY     NOT NULL,"\
                "CPU_AVG        INT                     NOT NULL,"\
                "RAM_AVG        INT                     NOT NULL,"\
                "IO_AVG        INT                     NOT NULL,"\
                "GPU_AVG        INT                     NOT NULL,"\
                "TEMP_AVG        INT                     NOT NULL,"\
                "NET_AVG         INT                     NOT NULL);";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create summary table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    sql = "CREATE TABLE IO_DEVICES("\
                "IP             TEXT                     NOT NULL,"\
                "IO_ID          TEXT                     NOT NULL,"\
                "IO_PERC        TEXT                     NOT NULL,"\
                "WRITES         INT                     NOT NULL,"\
                "PRIMARY KEY (IP, IO_ID));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create IO devices table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    sql = "CREATE TABLE NET_INTERFACES("\
                "IP             TEXT                     NOT NULL,"\
                "NET_ID         INT                     NOT NULL,"\
                "NET_USAGE      INT                     NOT NULL,"\
                "NET_SPEED      INT                     NOT NULL,"\
                "PRIMARY KEY (IP, NET_ID));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create net interfaces table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    sql = "CREATE TABLE CORES("\
                "IP             TEXT                     NOT NULL,"\
                "CORE_ID        INT                     NOT NULL,"\
                "TEMPERATURE    INT                     NOT NULL,"\
                "TEMP_PERC    INT                     NOT NULL,"\
                "POW_CONSUM     INT                     NOT NULL,"\
                "PRIMARY KEY (IP, CORE_ID));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create core table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    sql = "CREATE TABLE GPUS("\
                "IP             TEXT                     NOT NULL,"\
                "GPU_ID         INT                     NOT NULL,"\
                "TEMPERATURE    INT                     NOT NULL,"\
                "POW_CONSUM     INT                     NOT NULL,"\
                "MEM_USAGE      INT                     NOT NULL,"\
                "USAGE          INT                     NOT NULL,"\
                "PRIMARY KEY (IP, GPU_ID));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create gpu table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    /*To store core load information*/
    sql = "CREATE TABLE CORELOAD("\
                "IP             TEXT                     NOT NULL,"\
                "CORE_ID        INT                     NOT NULL,"\
                "LOAD           INT                     NOT NULL,"\
                "PRIMARY KEY (IP, CORE_ID));";

    /* Execute SQL statement */
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "Can't create coreload table.\n");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        db_close();
        return(-1);
    } else {
        //std::cout << "Table created successfully" << std::endl;
    }

    return(0);
}

/**
 * Fill auxiliary arrays to store temp information about nodes.
 * @param nodes List of node IP.
 */
void indexInit(std::list<std::string> nodes){
    for (std::string aux : nodes) {
        _nodeTime.insert(std::pair<std::string,int>(aux, 0));
        _lastnodeTime.insert(std::pair<std::string,int>(aux, 1));
    }
}

/**
 * Reset time counter to check if the nodes send us information or they are not available
 */
void *resetCounter() {
    while(1) {
        sleep(_time);
        for (auto row : _timer) {
            if(_timer[row.first] == 0)
                db_insert_CR(row.first, -1, -1, -1, -1, -1, -1);
            _timer[row.first] = 0;
        }
    }
}


/**
 * Function to insert information in table Configuration (static information).
 * @param id Node IP
 * @param ncpus Number of CPUs of a node.
 * @param ncores Number of Cores of a node.
 * @param nram Size of RAM of a node.
 * @param ngpus Number of GPUs of a node.
 * @param nio Number of IO devices of a node.
 * @param nnets Number of net interfaces of a node.
 * @return 0 if insert is OK, -1 if not.
 */
int db_insert_CONF(std::string id, int ncpus, int ncores, int nram, int ngpus, int nio, int nnets) {
    char *zErrMsg = nullptr;
    int rc;
    char sql[300];

    sprintf(sql, "INSERT INTO CONFIGURATION VALUES "\
        "(\"%s\", %d, %d, %d, %d, %d, %d);",
            id.c_str(), ncpus, ncores, nram, ngpus, nio, nnets);

    //std::cout << sql << std::endl;
    rc = sql_exec(db_global, sql, zErrMsg);

    if (rc != SQLITE_OK) {
        //fprintf(stderr, "SQL error inserting in configuration: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Insert into configuration ok" << std::endl;
    }

    /*If the insert is ok, we add this node to the list to initialize the logical timer*/
    _nodeTime.insert(std::pair<std::string,int>(id, 0));
    _lastnodeTime.insert(std::pair<std::string,int>(id,1));
    _initNodes.push_back(id);
    _timer.insert(std::pair<std::string,int>(id, 1)); // to check if nodes are running

    /*Thread to check heartbit*/
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thid, &attr, reinterpret_cast<void *(*)(void *)>(resetCounter), NULL);

    return (0);
}


/**
 * Function to insert information in table ComputeRow (dynamic information).
 * @param id Node IP.
 * @param iosat Usage of IO devices (average).
 * @param cpusat Usage of CPU.
 * @param ramsat Usage of RAM.
 * @param gpusat Usage of gpu (average between all gpus)
 * @return 0 if insert is OK, -1 if not.
 */
int db_insert_CR(std::string id, int iosat, int cpusat, int ramsat, int gpusat, int temp, int net) {
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    /*if(_nodeTime[id]%32 == 0)
        if (_nodeTime[id] > 0)
            updateSummary();
        else
            db_insertSummary(id, cpusat, ramsat, iosat, gpusat, temp, net);*/

    if(_nodeTime[id] < _n_items) {
        _nodeTime[id] += 1;
        sprintf(sql, "INSERT INTO COMPUTEROW (IP,TIME,IO_USAGE,CPU_USAGE,RAM_USAGE, GPU_USAGE, TEMP, NET) VALUES "\
        "(\"%s\", %d, %d, %d, %d, %d, %d, %d);",
                id.c_str(), _nodeTime[id], iosat, cpusat, ramsat, gpusat, temp, net);

        rc = sql_exec(db_global, sql, zErrMsg);

        if (rc != SQLITE_OK) {
            //fprintf(stderr, "SQL error inserting in computerow: %s\n", sql);
            sqlite3_free(zErrMsg);
            return (-1);
        } else {
            //std::cout << "Insert into computerow ok" << std::endl;
        }
    } else {
        //std::cout << "Reuse rows" << std::endl;
        _nodeTime[id] += 1;
        db_update_CR(id, _lastnodeTime[id], _nodeTime[id], iosat, cpusat, ramsat, gpusat, temp, net);
        _lastnodeTime[id] += 1;
    }

    _timer[id] += 1;

    return(0);
}

/**
 * Function to insert the independant load for each core
 * @param id node IP
 * @param core_load
 * @return 0 ok, -1 error.
 */
int db_insert_coreload(std::string id, std::vector<int> core_load){
    char *zErrMsg = nullptr;
    int rc;
    char sql [100];

    char * insert = db_query_coreload(id);
    if (strlen(insert) > 0){
        for (int i = 0; i < core_load.size(); i++) {
            sprintf(sql, "UPDATE CORELOAD SET LOAD = %d WHERE IP=\"%s\" AND CORE_ID=%i;", core_load[i], id.c_str(), i);

            rc = sql_exec(db_global, sql, zErrMsg);

            if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL error updating in coreload: %s\n", sql);
                sqlite3_free(zErrMsg);
                return (-1);
            } else {
                //std::cout << "Insert into coreload ok" << std::endl;
            }
        }
    }else {
        for (int i = 0; i < core_load.size(); i++) {
            sprintf(sql, "INSERT INTO CORELOAD (IP,CORE_ID,LOAD) VALUES "\
        "(\"%s\", %d, %d);", id.c_str(), i, core_load[i]);

            rc = sql_exec(db_global, sql, zErrMsg);

            if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL error inserting in coreload: %s\n", sql);
                sqlite3_free(zErrMsg);
                return (-1);
            } else {
                //std::cout << "Insert into coreload ok" << std::endl;
            }
        }
    }

    return(0);
}

/**
 * Function to insert data in table IOdevice.
 * @param id_node Node ID.
 * @param id_io IO device ID.
 * @param writes Percentage of writes over IO_PERC.
 * @param io_perc Percentage in operations of IO.
 * @return 0 if insert is OK, -1 if not.
 */
int db_insert_IOdev(std::string id_node, std::string id_io, int writes, int io_perc) {
    char *zErrMsg = nullptr;
    int rc;
    char sql[200];
    sprintf(sql, R"(INSERT INTO IO_DEVICES VALUES ("%s", "%s", %d, %d);)",
            id_node.c_str(), id_io.c_str(), io_perc, writes);


    rc = sql_exec(db_global, sql, zErrMsg);

    if (rc != SQLITE_OK) {
        //fprintf(stderr, "SQL error inserting in io_devices: %s\n", sql);
        sqlite3_free(zErrMsg);
        rc = db_update_IOdev(id_node, id_io, writes, io_perc);
        return (rc);
    } else {
        //std::cout << "Insert into io_devices ok" << std::endl;
    }

    return (0);
}

/**
 * Function to insert data in table Net Interfaces.
 * @param id_node Node IP.
 * @param id_net NET interface ID.
 * @param usage Usage of net interface.
 * @param speed Speed of total net interfaces of a node.
 * @return 0 if insert is OK, -1 if not.
 */
int db_insert_Net(std::string id_node, std::string id_net, int usage, int speed) {
    char *zErrMsg = nullptr;
    int rc;
    char sql[200];

    sprintf(sql, R"(INSERT INTO NET_INTERFACES VALUES ("%s", "%s", %d, %d);)",
            id_node.c_str(), id_net.c_str(), usage, speed);

    rc = sql_exec(db_global, sql, zErrMsg);

    if (rc != SQLITE_OK) {
        //fprintf(stderr, "SQL error inserting in net interfaces: %s\n", sql);
        sqlite3_free(zErrMsg);
        rc = db_update_Net(id_node, id_net, usage);
        return (rc);
    } else {
        //std::cout << "Insert into net interfaces ok" << std::endl;
    }

    return (0);
}

/**
 * Function to insert data in table Cores.
 * @param id_node Node IP.
 * @param id_core Core ID.
 * @param temperature Temperature of this core.
 * @param pow Power consumption.
 * @param temp_perc Percentage of temperature over max. temp. value.
 * @return 0 if insert is OK, -1 if not.
 */
int db_insert_Cores(std::string id_node, std::string id_core, int temperature, int pow, int temp_perc) {
    char *zErrMsg = nullptr;
    int rc;
    char sql[200];

    sprintf(sql, R"(INSERT INTO CORES VALUES ("%s", "%s", %d, %d, %d);)",
            id_node.c_str(), id_core.c_str(), temperature, temp_perc, pow);

    rc = sql_exec(db_global, sql, zErrMsg);

    if (rc != SQLITE_OK) {
        //fprintf(stderr, "SQL error inserting in Cores: %s\n", sql);
        sqlite3_free(zErrMsg);
        rc = db_update_Cores(id_node, id_core, temperature, pow, temp_perc);
        return (rc);
    } else {
        //std::cout << "Insert into cores ok" << std::endl;
    }

    return (0);
}

/**
 * Function to insert data in table GPUs.
 * @param id_node Node IP.
 * @param id_gpu GPU ID.
 * @param temperature Temperature of this GPU.
 * @param mem_usage Memory usage.
 * @param pow Power consumption
 * @param gen_usage Percentage of general usage.
 * @return 0 if insert is OK, -1 if not.
 */
int db_insert_Gpu(std::string id_node, std::string id_gpu, int temperature, int mem_usage, int pow, int gen_usage) {
    char *zErrMsg = nullptr;
    int rc;
    char sql[200];

    sprintf(sql, R"(INSERT INTO GPUS VALUES ("%s", "%s", %d, %d, %d, %d);)",
            id_node.c_str(), id_gpu.c_str(), temperature, mem_usage, pow, gen_usage);

    rc = sql_exec(db_global, sql, zErrMsg);

    if (rc != SQLITE_OK) {
        //fprintf(stderr, "SQL error inserting in Gpus: %s\n", sql);
        sqlite3_free(zErrMsg);
        rc = db_update_Gpu(id_node, id_gpu, temperature, mem_usage, pow, gen_usage);
        return (rc);
    } else {
        //std::cout << "Insert into gpus ok" << std::endl;
    }

    return (0);
}

/**
 * Function to insert data in table Summary.
 * @param id Node IP.
 * @param cpu CPU usage.
 * @param ram RAM usage.
 * @param io IO usage.
 * @param gpu GPU usage
 * @return 0 if insert is OK, -1 if not.
 */
int db_insertSummary(std::string id, int cpu, int ram, int io, int gpu, int temp, int net){
    char *zErrMsg = nullptr;
    int rc;
    char sql [200];
    sprintf(sql, "INSERT INTO SUMMARY (IP,CPU_AVG,RAM_AVG, IO_AVG, GPU_AVG, TEMP_AVG, NET_AVG) VALUES (\"%s\", %d, %d, %d, %d, %d, %d);",
            id.c_str(), cpu, ram, io, gpu, temp, net);

    rc = sql_exec(db_global, sql, zErrMsg);

    if (rc != SQLITE_OK) {
        //fprintf(stderr, "SQL error inserting in summary: %s\n", sql);
        sqlite3_free(zErrMsg);
        rc = db_update_summary(id, cpu, ram, io, gpu, temp, net);
        return (rc);
    }else{
        //std::cout << "Insert into summary ok" << std::endl;
    }
    return(0);
}




/**
 * Function to update row in table Computerow.
 * @param id Node IP.
 * @param lastTime Sample time which will be updated.
 * @param newTime New time for this sample.
 * @param iosat IO usage.
 * @param cpusat CPU usage.
 * @param ramsat RAM usage.
 * @paramg gpusat Gpu usage
 * @return 0 if update is OK, -1 if not.
 */
int db_update_CR(std::string id, int lastTime, int newTime, int iosat, int cpusat, int ramsat, int gpusat, int temp, int net) {
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    sprintf(sql, "UPDATE COMPUTEROW set TIME = %d, CPU_USAGE = %d, RAM_USAGE = %d, IO_USAGE = %d, GPU_USAGE = %d, TEMP = %d, NET = %d "
                 "WHERE IP = \"%s\" AND TIME = %d;", newTime, cpusat, ramsat, iosat, gpusat, temp, net, id.c_str(), lastTime);

    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        //fprintf(stderr, "SQL error updating computerow: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Update computerow OK" << std::endl;
    }

    return(0);
}

/**
 * Function to update row in table IO devices.
 * @param id_node Node IP.
 * @param id_io IO device ID.
 * @param writes Percentage of writes over io_perc.
 * @param io_perc Percentage of IO operations.
 * @return 0 if update is OK, -1 if not.
 */
int db_update_IOdev(std::string id_node, std::string id_io, int writes, int io_perc){
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    sprintf(sql, R"(UPDATE IO_DEVICES set WRITES = %d, IO_PERC = %d WHERE IP = "%s" AND IO_ID = "%s";)", writes, io_perc, id_node.c_str(), id_io.c_str());

    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        //fprintf(stderr, "SQL error updating io_devices: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Update io_devices OK" << std::endl;
    }

    return(0);
}

/**
 * Function to update row in table Net interfaces.
 * @param id_node Node IP.
 * @param id_net Net device ID.
 * @param usage Usage.
 * @return 0 if update is OK, -1 if not.
 */
int db_update_Net(std::string id_node, std::string id_net, int usage){
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    sprintf(sql, R"(UPDATE NET_INTERFACES set NET_USAGE = %d WHERE IP = "%s" AND NET_ID = "%s";)", usage, id_node.c_str(), id_net.c_str());
    
    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        //fprintf(stderr, "SQL error updating net interfaces: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Update net interfaces OK" << std::endl;
    }

    return(0);
}

/**
 * Function to update row in table Cores.
 * @param id_node Node IP.
 * @param id_core Core ID.
 * @param temperature Temperature of this core.
 * @param pow Power consumption.
 * @param temp_perc Percentaje of temperature over max. temp. value.
 * @return 0 if update is OK, -1 if not.
 */
int db_update_Cores(std::string id_node, std::string id_core, int temperature, int pow, int temp_perc){
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    sprintf(sql, R"(UPDATE CORES set TEMPERATURE = %d, TEMP_PERC = %d, POW_CONSUM = %d WHERE IP = "%s" AND CORE_ID = "%s";)", temperature, temp_perc, pow, id_node.c_str(), id_core.c_str());

    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        //fprintf(stderr, "SQL error updating cores: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Update cores OK" << std::endl;
    }

    return(0);
}

/**
 * Function to update row in table GPU.
 * @param id_node Node IP.
 * @param id_gpu GPU ID.
 * @param temperature Temperature.
 * @param mem_usage Memory usage.
 * @param pow Power consumption.
 * @param gen_usage General usage of this GPU.
 * @return 0 if update is OK, -1 if not.
 */
int db_update_Gpu(std::string id_node, std::string id_gpu, int temperature, int mem_usage, int pow, int gen_usage){
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    sprintf(sql, "UPDATE GPUS set TEMPERATURE = %d, MEM_USAGE = %d, POW_CONSUM = %d, USAGE = %d WHERE IP = \"%s\" "\
    "AND GPU_ID = \"%s\";", temperature, mem_usage, pow, gen_usage, id_node.c_str(), id_gpu.c_str());

    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        //fprintf(stderr, "SQL error updating gpu: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Update gpu OK" << std::endl;
    }

    return(0);
}

/**
 *Function to update row in table Summary
 * @param id Node IP.
 * @param cpusat CPU usage average.
 * @param ramsat RAM usage average.
 * @param iosat IO usage average.
 * @param gpusat GPU usage average
 * @return 0 if update is OK, -1 if not.
 */
int db_update_summary(std::string id, int cpusat, int ramsat, int iosat, int gpusat, int tempsat, int netsat) {
    char *zErrMsg = nullptr;
    int rc;
    char sql [300];

    sprintf(sql, "UPDATE SUMMARY set CPU_AVG = %d, RAM_AVG = %d, IO_AVG = %d, GPU_AVG = %d, TEMP_AVG = %d, NET_AVG = %d "
                 "WHERE IP = \"%s\";", cpusat, ramsat, iosat, gpusat, tempsat, netsat, id.c_str());

    rc = sql_exec(db_global, sql, zErrMsg);

    if( rc != SQLITE_OK ){
        //fprintf(stderr, "SQL error updating summary: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    } else {
        //std::cout << "Record created successfully" << std::endl;
    }

    return(0);
}




/**
 * Function to show all information stored in the specified table.
 * @param table Table to show.
 * @return 0 if query is OK, -1 if not.
 */
int db_query_table(char* table){
    int rc;
    char *sql;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec

    //Alternative
    asprintf(&sql, "SELECT * FROM '%s';", table);

    rc = sqlite3_prepare_v2(db_global, sql, strlen(sql), &res, nullptr); //query ckecked

    if(rc == SQLITE_OK) {
        //std::cout << "Query checked." << std::endl;
    } else {
        fprintf(stderr, "Failed to execute statement\n");
    }

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    }

    return(0);
}

/**
 * Function to show specific information about ComputeRow table. To ignore any field you must pass -1 as a param.
 * @param id Node IP.
 * @param time Sample time.
 * @param iouse IO usage.
 * @param cpuuse CPU usage
 * @param ramuse RAM usage.
 * @return 0 if query is OK, -1 if not.
 */
char* db_query_spec_CR(std::string id, int time, int iouse, int cpuuse, int ramuse){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);

    //Choose the best _option to select the data
    asprintf(&sql, "Select IP from COMPUTEROW");

    if(id.compare("-1") != 0){
        const char * aux = " where IP = ";
        asprintf(&sql, "%s%s\"%s\"", sql, aux, id.c_str());
    }
    if(time != -1){
        const char * aux = " where time = ";
        asprintf(&sql, "%s%s%d", sql, aux, time);
    }
    if (cpuuse != -1){
        const char * aux = " where CPU_USAGE < ";
        asprintf(&sql, "%s%s%d", sql, aux, cpuuse);
    }
    if (ramuse!= -1){
        const char * aux = " where RAM_USAGE < ";
        asprintf(&sql, "%s%s%d", sql, aux, ramuse);
    }
    if(iouse != -1){
        const char * aux = " where IO_USAGE < ";
        asprintf(&sql, "%s%s%d", sql, aux, iouse);
    }
    asprintf(&sql, "%s;", sql);

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query in computerow: %s\n", sql);
        sqlite3_free(zErrMsg);
        return NULL;
    }

    return (char*)local_buffer;
}

/**
 * Function to show specific information about Configuration table. To ignore any field you must pass -1 as a param.
 * @param id Node IP.
 * @param ncpus Numer of CPUs.
 * @param ncores Number of Cores.
 * @param nram Size of RAM.
 * @param ngpus Number of GPUs.
 * @param nio Number of IO devices.
 * @param nnets Number of net interfaces.
 * @return 0 if query is OK, -1 if not.
 */
int db_query_spec_Conf(std::string id, int ncpus, int ncores, int nram, int ngpus, int nio, int nnets){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);


    //Choose the best _option to select the data
    asprintf(&sql, "Select IP from CONFIGURATION");

    if(id.compare("-1") != 0){
        const char * aux = " where IP = ";
        asprintf(&sql, "%s%s\"%s\"", sql, aux, id.c_str());
    }
    if(ncpus != -1){
        const char * aux = " where NCPUS > ";
        asprintf(&sql, "%s%s%d", sql, aux, ncpus);
    }
    if (ncores != -1){
        const char * aux = " where NCORES > ";
        asprintf(&sql, "%s%s%d", sql, aux, ncores);
    }
    if (nram!= -1){
        const char * aux = " where NRAM > ";
        asprintf(&sql, "%s%s%d", sql, aux, nram);
    }
    if(ngpus != -1){
        const char * aux = " where NGPUS < ";
        asprintf(&sql, "%s%s%d", sql, aux, ngpus);
    }
    if(nio != -1){
        const char * aux = " where NIO < ";
        asprintf(&sql, "%s%s%d", sql, aux, nio);
    }
    if(nnets != -1){
        const char * aux = " where NNETS < ";
        asprintf(&sql, "%s%s%d", sql, aux, nnets);
    }
    asprintf(&sql, "%s;", sql);

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query in computerow: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    }

    return(0);
}

/**
 * Function to show all data stored in Computerow.
 * @return 0 if query is OK, -1 if not.
 */
char* db_query_all_CR(){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.

    asprintf(&sql, "Select * from COMPUTEROW;");

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global CR: %s\n", sql);
        sqlite3_free(zErrMsg);
        return NULL;
    }

    return (char*)local_buffer;
}

/**
 * Function to show all data stored in Configuration.
 * @return 0 if query is OK, -1 if not.
 */
char* db_query_all_Conf(){
    int rc;
    char *sql = nullptr;


    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    //Init localbuffer and index
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);
    memset(_buffer,0, SIZE_LOCAL_BUFFER);
    index_cb = 0;

    // ShowAll shows the static information from Configuration table.
    asprintf(&sql, "Select * from CONFIGURATION;");

    rc = sql_exec_buffer(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global Configuration: %s\n", sql);
        sqlite3_free(zErrMsg);
        return NULL;
    }

    free(sql);
    //sem_post(&sem);
    return (char*)_buffer;
}

/**
 * Function to show all data stored in IO devices.
 * @return 0 if query is OK, -1 if not.
 */
int db_query_all_IO(){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.

    asprintf(&sql, "Select * from IO_DEVICES;");

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global io devices: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    }

    return(0);
}

/**
 * Function to show all data stored in Cores.
 * @return 0 if query is OK, -1 if not.
 */
int db_query_all_Cores(){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.

    asprintf(&sql, "Select * from Cores;");

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global cores: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    }

    return(0);
}

/**
 * Function to show all data stored in GPUs.
 * @return 0 if query is OK, -1 if not.
 */
int db_query_all_GPU(){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.

    asprintf(&sql, "Select * from GPUS;");

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global gpus: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    }

    return(0);
}

/**
 * Function to show all data stored in Net interfaces.
 * @return 0 if query is OK, -1 if not.
 */
int db_query_all_Net(){
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.

    asprintf(&sql, "Select * from NET_INTERFACES;");

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global net interfaces: %s\n", sql);
        sqlite3_free(zErrMsg);
        return (-1);
    }

    return(0);
}

/**
 * Function to show all data stored, static and dynamic information, into all database tables. (INCOMPLETE)
 * @return 0 if query is OK, -1 if not.
 */
int db_query_all(){
    int rc;
    char *sql = nullptr;


    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);
    memset(_buffer,0, SIZE_LOCAL_BUFFER);

    //For each node, show its info and for each sub-field, show its info.
    for(auto pair : _nodeTime){
        asprintf(&sql, "Select COMPUTEROW.IP, CONFIGURATION.NCPUS, CONFIGURATION.NCORES, CONFIGURATION.NRAM, "\
        "CONFIGURATION.NGPUS, CONFIGURATION.NIO, CONFIGURATION.NNETS, COMPUTEROW.IO_USAGE, COMPUTEROW.GPU_USAGE "\
        "COMPUTEROW.CPU_USAGE, COMPUTEROW.RAM_USAGE from COMPUTEROW JOIN "\
        "CONFIGURATION ON COMPUTEROW.IP=CONFIGURATION.IP AND COMPUTEROW.IP=\"%s\";", pair.first.c_str());
        rc = sql_exec(db_global, sql, zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "Error executing query global: %s\n", sql);
            sqlite3_free(zErrMsg);
            return (-1);
        }
    }

    return(0);
}

/**
 * Function to show all dynamic information (configutation and computerow).
 * @return 0 if query is OK, -1 if not.
 */
char* db_query_all_Now(){
    int rc;
    char *sql = nullptr;


    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);
    memset(_buffer,0, SIZE_LOCAL_BUFFER);
    index_cb=0;

    //For each node, show its info and for each sub-field, show its info.
    for(auto pair : _nodeTime) {
        asprintf(&sql, "Select COMPUTEROW.IP, COMPUTEROW.IO_USAGE, "\
        "COMPUTEROW.CPU_USAGE, COMPUTEROW.RAM_USAGE, COMPUTEROW.GPU_USAGE, COMPUTEROW.TEMP, COMPUTEROW.NET from COMPUTEROW WHERE "\
        "COMPUTEROW.IP=\"%s\" AND COMPUTEROW.TIME=%d;", pair.first.c_str(), pair.second);

        rc = sql_exec_buffer(db_global, sql, zErrMsg);
        //std::cout << "SQL QUERY NOW: "<<sql<<std::endl;
        if (rc != SQLITE_OK) {
            //fprintf(stderr, "Error executing query global now: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return NULL;
        }
    }

    free(sql);
    //sem_post(&sem);
    return _buffer;//(char*)local_buffer;
}

/**
 * Function to show average information of dynamic data.
 * @return 0 if query is OK, -1 if not.
 */
char * db_query_all_Summary(){
    int rc;
    char *sql = nullptr;


    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);
    memset(_buffer,0, SIZE_LOCAL_BUFFER);
    index_cb = 0;

    asprintf(&sql, "Select * from SUMMARY;");

    rc = sql_exec_buffer(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query global summary: %s\n", sql);
        sqlite3_free(zErrMsg);
        return NULL;
    }

    free(sql);
    //sem_post(&sem);
    return (char*)_buffer;
}

/**
 * FlexMPI controller ask for free nodes to balance the workload.
 * @param np Number of processes requested
 * @param profile 0=CPU, 1=MEM, 2=IO
 * @return 0 if query is OK, -1 if not.
 */
char* db_query_allocate(int np, int profile) {
    int rc, counter = 0; //only return np nodes
    char *sql = nullptr;
    char *prof = (char *) calloc(3, sizeof(char));
    if (profile == CPU)
        strcpy(prof, "CPU_USAGE");
    else if (profile == MEM)
        strcpy(prof, "RAM_USAGE");
    else if (profile == IO)
        strcpy(prof, "IO_USAGE");
    else if (profile == GPU)
        strcpy(prof, "GPU_USAGE");

    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer, 0, SIZE_LOCAL_BUFFER);
    memset(_buffer,0, SIZE_LOCAL_BUFFER);
    index_cb = 0;

    //For each node, show its info and for each sub-field, show its info.
    //for(auto pair : _nodeTime) {
    //if(counter < np) {
    /*asprintf(&sql, "Select COMPUTEROW.IP, CONFIGURATION.NCORES from COMPUTEROW JOIN CONFIGURATION ON "\
"COMPUTEROW.IP=CONFIGURATION.IP AND COMPUTEROW.IP=\"%s\" AND COMPUTEROW.TIME=%d ORDER BY COMPUTEROW.%s ASC;",
             pair.first.c_str(), pair.second, prof);*/
    //asprintf(&sql, "Select distinct COMPUTEROW.IP, CONFIGURATION.NCORES from COMPUTEROW JOIN CONFIGURATION ON "\
 "COMPUTEROW.IP=CONFIGURATION.IP ORDER BY COMPUTEROW.TIME DESC, COMPUTEROW.%s ASC limit %i;", prof, np);
    //asprintf(&sql, "Select distinct COMPUTEROW.IP, (CONFIGURATION.NCORES - (1 + ((COMPUTEROW.CPU_USAGE * CONFIGURATION.NCORES) / 100))) "\
  "from COMPUTEROW JOIN CONFIGURATION ON COMPUTEROW.IP=CONFIGURATION.IP ORDER BY COMPUTEROW.TIME DESC, COMPUTEROW.%s ASC limit %i;", prof, np);
    //asprintf(&sql, "Select distinct COMPUTEROW.IP, CONFIGURATION.NCORES, COMPUTEROW.CPU_USAGE "\
  "from COMPUTEROW JOIN CONFIGURATION ON COMPUTEROW.IP=CONFIGURATION.IP GROUP BY COMPUTEROW.IP ORDER BY COMPUTEROW.TIME DESC, COMPUTEROW.%s ASC limit %i;", prof, np);
    asprintf(&sql, "Select distinct COMPUTEROW.IP, CONFIGURATION.NCORES, COMPUTEROW.CPU_USAGE "\
  "from COMPUTEROW JOIN CONFIGURATION ON COMPUTEROW.IP=CONFIGURATION.IP GROUP BY COMPUTEROW.IP ORDER BY COMPUTEROW.TIME DESC, COMPUTEROW.%s ASC;",
             prof);



    rc = sql_exec_buffer(db_global, sql, zErrMsg);
    //std::cout << "SQL QUERY NOW: "<<sql<<std::endl;
    if (rc != SQLITE_OK) {
        //fprintf(stderr, "Error executing query global now: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return NULL;
        //  }
        counter++;
        //}
    }

    if(strlen(_buffer) == 0)
    {
        free(sql);
        return NULL;
    }

    free(sql);
    //sem_post(&sem);
    return (char *) _buffer;
}

/**
 * Function to show core load
 * @return 0 if query is OK, -1 if not.
 */
char* db_query_coreload(std::string ip){
    int rc;
    char *sql = nullptr;

    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec
    // ShowAll shows the static information from Configuration table.
    //Init localbuffer
    memset(local_buffer,0, SIZE_LOCAL_BUFFER);
    memset(_buffer,0, SIZE_LOCAL_BUFFER);
    index_cb=0;

    // ShowAll shows the static information from Configuration table.
    asprintf(&sql, "Select * from CORELOAD WHERE IP=\"%s\";", ip.c_str());

    rc = sql_exec_buffer(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query core_load: %s\n", sql);
        sqlite3_free(zErrMsg);
        return NULL;
    }

    free(sql);
    //sem_post(&sem);
    return (char*)_buffer;
}




/**
 * Callback function to show data on stdout.
 * @param NotUsed Param of Callback function we never use.
 * @param argc Number of data.
 * @param argv Vector of data.
 * @param azColName Column name.
 * @return 0. Show information is always possible.
 */
int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i = 0;
    for(i = 0; i<argc; i++) {
        printf("%s = %s\t", azColName[i], argv[i] ? argv[i] : "NULL");

    }
    printf("\n");

    return(0);
}


/**
 * Callback function to show data on stdout.
 * @param NotUsed Param of Callback function we never use.
 * @param argc Number of data.
 * @param argv Vector of data.
 * @param azColName Column name.
 * @return 0. Show information is always possible.
 */
int callback_buffer(void *NotUsed, int argc, char **argv, char **azColName) {
    int i = 0;
    const char *s_unavailable = "-1";
    int unavailable = 0;

    for(auto dir : _timer){
        if(strcmp(dir.first.c_str(), argv[0]) == 0){
            if(dir.second == 0)
                unavailable = 1;
        }
        break;
    }

    for(i = 0; i<argc; i++) {
        /*Uncomment to debug*/
        //printf("%s = %s\t", azColName[i], argv[i] ? argv[i] : "NULL");

        /*Data must be copied into buffer to reduce its size*/
        if (i != 0 && unavailable == 0) {
            int len = strlen(argv[i]);
            unsigned char len_s;
            len_s = (unsigned char) len;
            memcpy(&local_buffer[index_cb], &len_s, sizeof(len_s));
            index_cb += 1;
            memcpy(&local_buffer[index_cb], argv[i], strlen(argv[i]));
            index_cb += strlen(argv[i]);
        } else if (i != 0 && unavailable != 0){
            int len = strlen(s_unavailable);
            unsigned char len_s;
            len_s = (unsigned char) len;
            memcpy(&local_buffer[index_cb], &len_s, sizeof(len_s));
            index_cb += 1;
            memcpy(&local_buffer[index_cb], s_unavailable, strlen(s_unavailable));
            index_cb += strlen(s_unavailable);
        }else{
            int len = strlen(argv[i]);
            unsigned char len_s;
            len_s = (unsigned char) len;
            memcpy(&local_buffer[index_cb], &len_s, sizeof(len_s));
            index_cb += 1;
            memcpy(&local_buffer[index_cb], argv[i], strlen(argv[i]));
            index_cb += strlen(argv[i]);
        }
    }

    printf("\n");

    return(0);
}

/**
 * Callback function to show data on stdout.
 * @param NotUsed Param of Callback function we never use.
 * @param argc Number of data.
 * @param argv Vector of data.
 * @param azColName Column name.
 * @return 0. Show information is always possible.
 */
int callback_buffer_string(void *NotUsed, int argc, char **argv, char **azColName) {
    int i = 0;
    const char *s_unavailable = "-1";
    int unavailable = 0;

    for(auto dir : _timer){
        if(strcmp(dir.first.c_str(), argv[0]) == 0){
            if(dir.second == 0)
                unavailable = 1;
        }
        break;
    }

    for(i = 0; i<argc; i++) {
        /*Uncomment to debug*/
        //printf("%s = %s\t", azColName[i], argv[i] ? argv[i] : "NULL");

        /*Data must be copied into buffer to reduce its size*/
        if (i != 0 && unavailable == 0) {
            strcat(&_buffer[index_cb], argv[i]);
            index_cb += strlen(argv[i]);
            strcat(&_buffer[index_cb], ":");
            index_cb++;
        } else if (i != 0 && unavailable != 0) {
            int len = strlen(s_unavailable);
            strcat(&_buffer[index_cb], s_unavailable);
            index_cb += len;
            strcat(&_buffer[index_cb], ":");
            index_cb++;
        } else{
            strcat(&_buffer[index_cb], argv[i]);
            index_cb += strlen(argv[i]);
            strcat(&_buffer[index_cb], ":");
            index_cb++;
        }
    }

    //printf("\n");

    return(0);
}


/**
 * Callback function to get avg data from table Summary.
 * @param NotUsed Param of Callback function we never use.
 * @param argc Number of data.
 * @param argv Vector of data.
 * @param azColName Column name.
 * @return Data average.
 */
int callback_avg(void *NotUsed, int argc, char **argv, char **azColName) {
    long avg = 0;
    char * pEnd;
    //std::cout << "RES AVG = " << argv[0] << std::endl;
    avg = argv[0] ? strtol(argv[0],&pEnd, 10) : 0;
    //std::cout << "AVG = " << avg << std::endl;
    return static_cast<int>(avg);
}



/**
 * Function to execute sql statements into a DB.
 * @param db Database
 * @param sql SQL statement.
 * @param zErrMsg Pointer to a possible errors.
 * @return result of sql_exec function.
 */
int sql_exec(sqlite3 *db, const char *sql, char *zErrMsg) {
    int res = sqlite3_exec(db, sql, callback, nullptr, &zErrMsg);
    return(res);
}


/**
 * Function to execute sql statements into a DB.
 * @param db Database
 * @param sql SQL statement.
 * @param zErrMsg Pointer to a possible errors.
 * @return result of sql_exec function.
 */
int sql_exec_buffer(sqlite3 *db, const char *sql, char *zErrMsg) {
    int res = sqlite3_exec(db, sql, callback_buffer_string, nullptr, &zErrMsg);
    return(res);
}


/**
 *
 * Perform an online backup of database pDb to the database file named
 * by zFilename. This function copies 5 database pages from pDb to
 * zFilename, then unlocks pDb and sleeps for 250 ms, then repeats the
 * process until the entire database is backed up.
 *
 * The third argument passed to this function must be a pointer to a progress
 * function. After each set of 5 pages is backed up, the progress function
 * is invoked with two integer parameters: the number of pages left to
 * copy, and the total number of pages in the source file. This information
 * may be used, for example, to update a GUI progress bar.
 *
 * While this function is running, another thread may use the database pDb, or
 * another process may access the underlying database file via a separate
 * connection.
 *
 * If the backup process is successfully completed, SQLITE_OK is returned.
 * Otherwise, if an error occurs, an SQLite error code is returned.
 * @param pDb Database to back up
 * @param zFilename Name of file to back up to
 * @param xProgress progress function to invoque
 * @return
 */
int backupDb(
        //sqlite3 *pDb,               /* Database to back up */
        const char *zFilename/*,      /* Name of file to back up to */
        //void(*xProgress)(int, int)  /* Progress function to invoke */
){
    int rc;                     /* Function return code */
    sqlite3 *pFile;             /* Database connection opened on zFilename */
    sqlite3_backup *pBackup;    /* Backup handle used to copy data */

    /* Open the database file identified by zFilename. */
    rc = sqlite3_open(zFilename, &pFile);
    if( rc==SQLITE_OK ){

        /* Open the sqlite3_backup object used to accomplish the transfer */
        pBackup = sqlite3_backup_init(pFile, "main", /*pDb*/db_global, "main");
        if( pBackup ){

            /* Each iteration of this loop copies 5 database pages from database
            ** pDb to the backup database. If the return value of backup_step()
            ** indicates that there are still further pages to copy, sleep for
            ** 250 ms before repeating. */
            do {
                rc = sqlite3_backup_step(pBackup, 5);
                /*xProgress(
                        sqlite3_backup_remaining(pBackup),
                        sqlite3_backup_pagecount(pBackup)
                );*/
                if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ){
                    sqlite3_sleep(250);
                }
            } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

            /* Release resources allocated by backup_init(). */
            (void)sqlite3_backup_finish(pBackup);
        }
        rc = sqlite3_errcode(pFile);
    }

    /* Close the database connection opened on database file zFilename
    ** and return the result of this function. */
    (void)sqlite3_close(pFile);
    return rc;
}



/**
 * Unused: This function launch 3 threads to: show information, take user commands from keyboard, update information.
 */
void processDB(){

    /*std::thread t1(optionShowInfo);
    std::thread t2(showInfo);
    std::thread t3(updateSummary);
    t1.join();
    t2.join();
    t3.join();*/
}

/**
 * This function shows information on stdout.
 */
void showInfo(){
    while(_option != "quit"){
        sleep(3);
        /* The options haven't got the initial 's' because it's ignored at cin.getline */
        if(_option.find("id") != std::string::npos){
            //std::cout <<"SID"<<std::endl;
            StringVector sv = explode(_option, ' ');
            std::string id = sv.at(1);
            clearScreen();
            db_query_spec_CR(id, -1, -1, -1, -1);
        }else if(_option.find("ramsat") != std::string::npos){
            //std::cout <<"SRAMSAT"<<std::endl;
            StringVector sv = explode(_option, ' ');
            clearScreen();
            int ram = std::stoi(sv.at(1));
            db_query_spec_CR("-1", -1, -1, -1, ram);
        } else if(_option.find("cpusat") != std::string::npos){
            //std::cout <<"SCPUSAT"<<std::endl;
            StringVector sv = explode(_option, ' ');
            clearScreen();
            int cpu = std::stoi(sv.at(1));
            db_query_spec_CR("-1", -1, -1, cpu, -1);
        } else if(_option.find("iosat") != std::string::npos){
            //std::cout <<"SIOSAT"<<std::endl;
            StringVector sv = explode(_option, ' ');
            clearScreen();
            int io = std::stoi(sv.at(1));
            db_query_spec_CR("-1", -1, io, -1, -1);
        }else if(_option.find("gpu") != std::string::npos){
            //std::cout <<"SGPU"<<std::endl;
            StringVector sv = explode(_option, ' ');
            clearScreen();
            int gpu = std::stoi(sv.at(1));
            db_query_spec_Conf("-1", -1, -1, -1, gpu, -1, -1);
        }else if(_option.find("all") != std::string::npos){
            clearScreen();
            db_query_table(const_cast<char *>("Computerow"));
        }else if(_option.find("summary") != std::string::npos){
            clearScreen();
            db_query_table(const_cast<char *>("Summary"));
        }else if(_option.find("total") != std::string::npos){
            clearScreen();
            std::cout<<"Showing all information about all nodes."<<std::endl;
            db_query_all();
        }else if(_option.find("configuration") != std::string::npos) {
            clearScreen();
            db_query_table(const_cast<char *>("configuration"));
        }else if(_option.find("pause") != std::string::npos){
            std::string aux;
            std::cin.ignore();
            getline(std::cin, aux);
            _option = "all";
        }
    }
}

/**
 * It parses data from keyboard.
 */
void optionShowInfo(){
    while (_option != "quit"){
        std::cin.ignore();
        //std::cin >> _option;
        getline(std::cin, _option, '\n');

        //std::cout << "Option captured: " << _option << std::endl;
    }
}

/**
 * Clear the screen each 10 seconds.
 */
void clearScreen(){
    for (int i = 0; i < 10; i++)
        std::cout << "\n\n\n" << std::endl;
}

/**
 * Split the input string from keyboard into comands and parameters.
 * @param str Input string.
 * @param separator Char separator.
 * @return Vector of strings, including comands and parameters.
 */
StringVector explode(const std::string & str, char separator) {
    StringVector  result;
    size_t pos1 = 0;
    size_t pos2 = 0;
    while ( pos2 != str.npos )
    {
        pos2 = str.find(separator, pos1);
        if ( pos2 != str.npos )
        {
            if ( pos2 > pos1 )
                result.push_back( str.substr(pos1, pos2-pos1) );
            pos1 = pos2+1;
        }
    }
    result.push_back( str.substr(pos1, str.size()-pos1) );
    return result;
}

/**
 * Function to update summary information.
 */
void updateSummary(){
    //sleep(10);
    char sql [200];
    char *zErrMsg = nullptr;
    int cpu, ram, io, gpu, temp, net;
    //std::cout << "Mostrando Resumen" << std::endl;
    //db_query_all(const_cast<char *>("SUMMARY"));
    //sleep(5);
    while(_option != "quit") {
        for (std::string aux : _initNodes) {
            sprintf(sql,"SELECT avg(CPU_USAGE) FROM COMPUTEROW WHERE IP = %s;", aux.c_str());
            cpu = sqlite3_exec(db_global, sql, callback_avg, &cpu, &zErrMsg);
            sprintf(sql,"SELECT avg(RAM_USAGE) FROM COMPUTEROW WHERE IP = %s;", aux.c_str());
            ram = sqlite3_exec(db_global, sql, callback_avg, &ram, &zErrMsg);
            sprintf(sql,"SELECT avg(IO_USAGE) FROM COMPUTEROW WHERE IP = %s;", aux.c_str());
            io = sqlite3_exec(db_global, sql, callback_avg, &io, &zErrMsg);
            sprintf(sql,"SELECT avg(GPU_USAGE) FROM COMPUTEROW WHERE IP = %s;", aux.c_str());
            gpu = sqlite3_exec(db_global, sql, callback_avg, &gpu, &zErrMsg);
            sprintf(sql,"SELECT avg(TEMP_USAGE) FROM COMPUTEROW WHERE IP = %s;", aux.c_str());
            temp = sqlite3_exec(db_global, sql, callback_avg, &temp, &zErrMsg);
            sprintf(sql,"SELECT avg(NET_USAGE) FROM COMPUTEROW WHERE IP = %s;", aux.c_str());
            net = sqlite3_exec(db_global, sql, callback_avg, &net, &zErrMsg);
            //printf("Media CPU = %d", result);
            db_update_summary(aux, cpu, ram, io, gpu, temp, net);
        }
    }
}

/**
 * It converts a short value to unsigned char
 * */
int convert_short_int_to_char(char * buffer, unsigned short int number){
    buffer[0] = (unsigned char)(number>>8);
    buffer[1]  = number & 0xff;

    return 0;
}

/**
 * Time to confirm that a node is disconnected
 */
void setDefaultTimer(time_t time){
    _time = time;
}


/*int db_query_count(char *table, int id) {
    int rc;
    char *sql;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec

    count = 0;
    //Alternative
    asprintf(&sql, "SELECT count(*) FROM '%s' where ID = %d;", table, id);

    rc = sqlite3_prepare_v2(db_global, sql, strlen(sql), &res, nullptr); //query ckecked

    if(rc == SQLITE_OK) {
        fprintf(stdout, "Query checked.\n");
    } else {
        fprintf(stderr, "Failed to execute statement\n");
    }

    rc = sql_exec_count(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return (1);
    }

    return(count);
}*/

/* The parameters that user don't want to use must be -1 */
/*int db_query_spec_CR(int id, int time, int ncpus, int ncores, int nram, int ngpus, int nio, int nnets, int netspeed,
                     int cpusat, int ramsat, int iosat, bool showAll) {
    int rc;
    char *sql = nullptr;
    sqlite3_stmt *res; // reserved to prepared statements
    char *zErrMsg = nullptr; //reserved to alternate exec


    //Choose the best _option to select the data
    if (showAll)
        asprintf(&sql, "Select * from COMPUTEROW");
    else
        asprintf(&sql, "Select IP from COMPUTEROW");

    if(id != -1){
        const char * aux = " where ID = ";
        asprintf(&sql, "%s%s%d", sql, aux, id);
    }
    if(time != -1){
        const char * aux = " where time = ";
        asprintf(&sql, "%s%s%d", sql, aux, time);
    }
    if (ncpus != -1){
        const char * aux = " where NCPUS = ";
        asprintf(&sql, "%s%s%d", sql, aux, ncpus);
    }
    if(ncores != -1){
        const char * aux = " where NCORES = ";
        asprintf(&sql, "%s%s%d", sql, aux, ncores);
    }
    if (nram != -1){
        const char * aux = " where NRAM = ";
        asprintf(&sql, "%s%s%d", sql, aux, nram);
    }
    if (ngpus != -1){
        const char * aux = " where NGPUS = ";
        asprintf(&sql, "%s%s%d", sql, aux, ngpus);
    }
    if(nio != -1){
        const char * aux = " where NIO = ";
        asprintf(&sql, "%s%s%d", sql, aux, nio);
    }
    if(nnets != -1){
        const char * aux = " where NNETS = ";
        asprintf(&sql, "%s%s%d", sql, aux, nnets);
    }
    if(netspeed != -1){
        const char * aux = " where NET_SPEED = ";
        asprintf(&sql, "%s%s%d", sql, aux, netspeed);
    }
    if (cpusat != -1){
        const char * aux = " where CPU_USAGE < ";
        asprintf(&sql, "%s%s%d", sql, aux, cpusat);
    }
    if (ramsat != -1){
        const char * aux = " where RAM_USAGE < ";
        asprintf(&sql, "%s%s%d", sql, aux, ramsat);
    }
    if(iosat != -1){
        const char * aux = " where IO_USAGE < ";
        asprintf(&sql, "%s%s%d", sql, aux, iosat);
    }
    asprintf(&sql, "%s;", sql);

    rc = sql_exec(db_global, sql, zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "Error executing query in computerow: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return (1);
    }

    return(0);
}*/


/*int callbackCount(void *NotUsed, int argc, char **argv, char **azColName) {
    count = argc;
    printf("\n");
    return(0);
}*/


/*int sql_exec_count(sqlite3 *db, const char *sql, char *zErrMsg) {
    int res = sqlite3_exec(db, sql, callbackCount, nullptr, &zErrMsg);
    return(res);
}*/


