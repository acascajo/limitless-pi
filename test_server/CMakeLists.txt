#Proyecto_2017
cmake_minimum_required (VERSION 2.8)
project (Test_server)

#if(DEBUG_DAEMON)
#	MESSAGE)
#endif(DEBUG_DAEMON)
find_package (Threads)
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/../include" )
FILE(GLOB MATH_SOURCES "../include/*.h")
source_group(include FILES ${MATH_SOURCES})

######## CUDA ########
#find_package(CUDA REQUIRED)
#message(${CUDA_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${CUDA_TOOKIT_ROOT_DIR})

FIND_PATH(LIBDL_INCLUDE_DIR dlfcn.h)

SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_C++_COMPILER g++)

#!!!El flag -g es para las pruebas con el valgrind, pero luego hay que quitarlo
#list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -D_GNU_SOURCE -g")

# Sin el parametro para valgrind, gdb
list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -D_GNU_SOURCE -g -ldl")

#### Debug options
add_definitions(-DDEBUG_DAEMON)

# Debug time
add_definitions(-DDAEMON_SERVER_DEBUG)
#list(APPEND CMAKE_CXX_FLAGS "-DDEBUG_TIME")

# Debug temp
#add_definitions(-DDEBUG_TEMP)
#list(APPEND CMAKE_CXX_FLAGS "-D DEBUG_TEMP")

#Debug sample
#list(APPEND CMAKE_CXX_FLAGS "-D DEBUG_SAMPLE")
#Debig sample
#add_definitions(-DDAEMON_SERVER_DEBUG)



# Debug execution
#list(APPEND CMAKE_CXX_FLAGS "-D DEBUG_EXECUTION")
message(${CMAKE_SOURCE_DIR})
list(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE -g")
add_executable(Test_server  ../src/cliente_monitor.cpp
				../src/cpu_info.cpp
				../src/devices_info.cpp
				../src/dir_path.cpp
				../src/memory_info.cpp
				../src/net_info.cpp
				../src/network_data.cpp
				../src/Packed_sample.cpp
				../src/sample_info.cpp
				../src/servidor_monitor.cpp
				../src/system_features.cpp
				../src/temp_info.cpp
				../src/common.cpp
				../src/library.cpp
				../src/sqlite3.c
				../src/server_data.cpp
				../src/connector.cpp
				../include/connector.h
        test_server.cpp)

target_link_libraries(Test_server ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} -lcurl)
#add_executable(writer writer_example.c)
#add_executable(reader reader_example.c)
