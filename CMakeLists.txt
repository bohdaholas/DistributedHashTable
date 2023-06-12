cmake_minimum_required(VERSION 3.25)
project(all)

set(CMAKE_CXX_STANDARD 23)

include(common.cmake)

get_filename_component(proto_file "proto/dds.proto" ABSOLUTE)
get_filename_component(proto_path "${proto_file}" PATH)

find_package(Boost 1.71.0 COMPONENTS program_options system thread REQUIRED)
find_package(ppconsul REQUIRED)

set(dds_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/dds.pb.cc")
set(dds_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/dds.pb.h")
set(dds_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/dds.grpc.pb.cc")
set(dds_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/dds.grpc.pb.h")
add_custom_command(
        OUTPUT "${dds_proto_srcs}" "${dds_proto_hdrs}" "${dds_grpc_srcs}" "${dds_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto_file}"
        DEPENDS "${proto_file}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(dds_grpc_proto ${dds_grpc_srcs} ${dds_grpc_hdrs} ${dds_proto_srcs} ${dds_proto_hdrs})
target_link_libraries(dds_grpc_proto ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

add_subdirectory(dds)
add_subdirectory(dds_client)
