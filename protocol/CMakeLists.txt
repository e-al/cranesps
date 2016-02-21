cmake_minimum_required(VERSION 2.8)
project(protocol)

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS dfile.proto election.proto dmember.proto crane.proto)

add_library(${PROJECT_NAME} STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})