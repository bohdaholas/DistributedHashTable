// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include "dds_client.h"

using std::cout, std::endl;

DDS_Client &DDS_Client::get_instance(config_options_t &opt) {
    static DDS_Client dds_client{opt};
    return dds_client;
}

DDS_Client::DDS_Client(config_options_t &opt)
        : opt{opt}, agent{consul} {}

DDS_Map& DDS_Client::get_map() {
    static DDS_Map dds_map(consul, opt);
    return dds_map;
}
