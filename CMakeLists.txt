#Proyecto_2017
cmake_minimum_required (VERSION 2.8)
project (DaeMon)

find_package (Threads)
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/include" )
FILE(GLOB MATH_SOURCES "include/*.h")
source_group(include FILES ${MATH_SOURCES})


######## CUDA ########
#find_package(CUDA REQUIRED)
#message(${CUDA_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${CUDA_TOOKIT_ROOT_DIR})

FIND_PATH(LIBDL_INCLUDE_DIR dlfcn.h)

#SET(LIBDL_NAMES dl libdl ltdl libltdl)
#FIND_LIBRARY(LIBDL_LIBRARY NAMES ${LIBDL_NAMES})

# handle the QUIETLY and REQUIRED arguments and set LIBDL_FOUND to TRUE if 
# all listed variables are TRUE
#INCLUDE(FindPackageHandleStandardArgs)
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibDL DEFAULT_MSG LIBDL_LIBRARY LIBDL_INCLUDE_DIR)

#IF(LIBDL_FOUND)
 # SET( LIBDL_LIBRARIES ${LIBDL_LIBRARY} )
#ELSE(LIBDL_FOUND)
 # SET( LIBDL_LIBRARIES )
#ENDIF(LIBDL_FOUND)

#MARK_AS_ADVANCED(LIBDL_LIBRARY LIBDL_INCLUDE_DIR)

######################


SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_C++_COMPILER g++)

#!!!El flag -g es para las pruebas con el valgrind, pero luego hay que quitarlo
#list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -D_GNU_SOURCE -g")

# Sin el parametro para valgrind, gdb
list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -D_GNU_SOURCE -ldl")

################ Debug options ##############
###add_definitions(-DDEBUG_DAEMON)

# Debug time
#add_definitions(-DDEBUG_TIME)
#list(APPEND CMAKE_CXX_FLAGS "-DDEBUG_TIME")

# Debug temperature
#add_definitions(-DDEBUG_TEMP)
#list(APPEND CMAKE_CXX_FLAGS "-D DEBUG_TEMP")

#Debug sample
#list(APPEND CMAKE_CXX_FLAGS "-D DEBUG_SAMPLE")
#Debig sample
#add_definitions(-DDAEMON_SERVER_DEBUG)

# Debug log buffer bitmap
add_definitions(-DDEBUG_LBBM)

#Debug bit map
#add_definitions(-DDEBUG_BIT_MAP)

#Debug GPU
#add_definitions(-DDEBUG_GPU)

# Debug execution
#list(APPEND CMAKE_CXX_FLAGS "-D DEBUG_EXECUTION")
message(${CMAKE_SOURCE_DIR})
list(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE -g")
file(GLOB Src_Sources RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src/*")
add_executable(DaeMon ${Src_Sources} )

target_link_libraries(DaeMon ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} -lcurl)
#add_executable(writer writer_example.c)
#add_executable(reader reader_example.c)
