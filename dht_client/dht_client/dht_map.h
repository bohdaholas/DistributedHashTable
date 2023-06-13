#ifndef ALL_MAP_WRAPPER_H
#define ALL_MAP_WRAPPER_H

#include <unordered_map>
#include <string>
#include <grpcpp/grpcpp.h>
#include <ppconsul/catalog.h>
#include <ppconsul/kv.h>
#include "dds.grpc.pb.h"
#include "config_parser.h"

class DDS_Map {
public:
    DDS_Map(const ppconsul::Consul &consul,
            const config_options_t &opt);
    int get(const std::string& key);
    void put(const std::string& key, int value);
    int size();
    bool empty();
    void remove(const std::string &key);
    void clear();
private:
    const ppconsul::Consul &consul;
    const config_options_t &opt;
};

#endif //ALL_MAP_WRAPPER_H
