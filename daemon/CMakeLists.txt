cmake_minimum_required(VERSION 2.8)
project(daemon)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(ZMQ REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Boost 1.53 COMPONENTS program_options REQUIRED)

include_directories(src)
include_directories(../utils)
include_directories(../crane-core/src)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src CACHE INTERNAL "${PROJECT_NAME}: Include directories" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -O -ldl")
file(GLOB_RECURSE DAEMON_SOURCES "*.cpp")
file(GLOB_RECURSE DAEMON_HEADERS "*.h")
message(STATUS ${DAEMON_SOURCES} ${DAEMON_HEADERS})

MESSAGE(STATUS "DAEMON_SOURCES: " ${DAEMON_SOURCES})
MESSAGE(STATUS "DAEMON_HEADERS " ${DAEMON_HEADERS})

add_executable(${PROJECT_NAME} ${DAEMON_SOURCES} ${DAEMON_HEADERS})

target_link_libraries(${PROJECT_NAME} ${ZMQ_LIBRARIES})
target_link_libraries(${PROJECT_NAME} protocol)
target_link_libraries(${PROJECT_NAME} crane-core)
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

