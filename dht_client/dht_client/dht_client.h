// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef DDS_CLIENT_H
#define DDS_CLIENT_H

#include <ppconsul/agent.h>
#include <random>
#include "config_parser.h"
#include "dht_map.h"

class DDS_Client {
public:
    static DDS_Client& get_instance(config_options_t &opt);
    DDS_Map& get_map();
private:
    DDS_Client(config_options_t &opt);
    DDS_Client(const DDS_Client&) = delete;
    DDS_Client& operator=(const DDS_Client&) = delete;

    config_options_t opt;
    ppconsul::Consul consul;
    ppconsul::agent::Agent agent;
};

#endif //DDS_CLIENT_H
