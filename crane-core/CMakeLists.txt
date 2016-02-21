cmake_minimum_required(VERSION 2.8)
project(crane-core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(Protobuf REQUIRED)


include_directories(../utils)
include_directories(src)
include_directories(${PROTOBUF_INCLUDE_DIRS})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src CACHE INTERNAL "${PROJECT_NAME}: Include directories" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -O")
file(GLOB_RECURSE CRANE_CORE_SOURCES "*.cpp")
file(GLOB_RECURSE CRANE_CORE_HEADERS "*.h")

add_library(${PROJECT_NAME} STATIC ${CRANE_CORE_SOURCES} ${CRANE_CORE_HEADERS})

target_link_libraries(${PROJECT_NAME} protocol)
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIBRARIES})
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")