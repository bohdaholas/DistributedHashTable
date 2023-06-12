file(GLOB_RECURSE SOURCES "dds_controller/*.cpp" "config_parser/*.cpp" "main.cpp")
add_executable(dds ${SOURCES})
target_include_directories(dds PRIVATE dds_controller config_parser ${Boost_INCLUDE_DIR})
target_link_libraries(dds PRIVATE
        Boost::program_options
        Boost::system
        Boost::thread
        ppconsul
        dds_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})
