project(SimpleXSim)
cmake_minimum_required(VERSION 2.8)

## Compiler options
# Include debug symbols
set(CMAKE_CXX_FLAGS "-g")
# Compile using the c++11 standard
set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -std=c++11")
# Enable all warnings
set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -Wall")


## Custom CMake Modules will be in this directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Search for SST
find_package(SST 7.1.0 REQUIRED)
# Search for Jsoncpp (a library to parse JSON files)
find_package(Jsoncpp REQUIRED)

# The vairable JSONCPP_INCLUDE_DIR is set when "find_package(Jsoncpp REQUIRED)" is called
#  The code that is called by find_package can be found in the project directory in "cmake/FindJsoncpp.cmake"
include_directories(${JSONCPP_INCLUDE_DIR})

# The vairable SST_INCLUDE_DIRS is set when "find_package(SST 6.1.0 REQUIRED)" is called
#  The code that is called by find_package can be found in the project directory in "cmake/FindSST.cmake"
include_directories(${SST_INCLUDE_DIRS})

set(CMAKE_BUILD_TYPE Debug)

include_directories(sst/include)
#########################################
#				SST library				#
#########################################
# Create our library
add_library(
        XSim
	SHARED

	sst/src/sst_core/sst_core.cpp
	sst/include/sst_core/sst_core.hpp

	sst/src/sst_core/sst_memory.cpp
	sst/include/sst_core/sst_memory.hpp

	sst/src/sst_core/xsim_sst_interface.cpp
)

target_compile_options(XSim PUBLIC ${SST_CFLAGS} ${SST_LDFLAGS})
target_include_directories(XSim PUBLIC ${SST_INCLUDE_DIRS})
target_compile_definitions(XSim PUBLIC ${SST_DEFINITIONS})
#add_dependencies(${PROJECT_NAME}
#	${PROJECT_NAME}_instruction_set
#	${PROJECT_NAME}_memory
#	${PROJECT_NAME}_state
#	)
# Remove all the SST warnings about deprecated functions.
target_compile_options(XSim PUBLIC "-Wno-deprecated")
#target_link_libraries(
#	${PROJECT_NAME}
#	${PROJECT_NAME}_CORE
#	${PROJECT_NAME}_instruction_set
#	${JSONCPP_LIBRARIES}
#	${PROJECT_NAME}_memory
#	${PROJECT_NAME}_state
#	)
