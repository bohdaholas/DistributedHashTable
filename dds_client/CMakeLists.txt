file(GLOB_RECURSE SOURCES "dds_client/*.cpp" "config_parser/*.cpp" "main.cpp")
add_executable(dds_client ${SOURCES})
target_include_directories(dds_client PRIVATE dds_client config_parser ${Boost_INCLUDE_DIR})
target_link_libraries(dds_client PRIVATE
        Boost::program_options
        Boost::system
        Boost::thread
        ppconsul
        dds_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})
