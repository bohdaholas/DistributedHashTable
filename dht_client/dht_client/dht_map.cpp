#include "dht_map.h"
#include "cluster_management.h"

using dds::MapKey;
using dds::MapValue;
using dds::KeyValue;
using dds::Void;
using dds::Int;
using dds::Bool;
using std::cout, std::cerr, std::endl;

DDS_Map::DDS_Map(const ppconsul::Consul &consul,
                 const config_options_t &opt)
                : consul(consul), opt(opt) {}

void DDS_Map::put(const std::string& key, int value) {
    auto stubs_ptrs = dds::get_stubs_by_key<dds::MapService>(consul, opt, key);
    KeyValue key_value;
    Void void_type;
    key_value.set_map_key(key);
    key_value.set_map_value(value);

//    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stubs_ptrs[0]->SetValue(&context, key_value, &void_type);
        if (!status.ok()) {
            dds::print_grpc_error(status);
        }
//    }
}

int DDS_Map::get(const std::string &key) {
    auto stubs_ptrs = dds::get_stubs_by_key<dds::MapService>(consul, opt, key);

    MapKey map_key;
    map_key.set_map_key(key);
    MapValue map_value;

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->GetValue(&context, map_key, &map_value);
        if (!status.ok()) {
            dds::print_grpc_error(status);
        }
    }
    return map_value.map_value();
}

int DDS_Map::size() {
    auto stub_ptr = dds::get_single_stub<dds::MapService>(consul, opt);

    Void void_type;
    Int keys_num;

    grpc::ClientContext context;
    grpc::Status status = stub_ptr->Size(&context, void_type, &keys_num);
    if (!status.ok()) {
        dds::print_grpc_error(status);
    }
    return keys_num.num();
}

bool DDS_Map::empty() {
    auto stub_ptr = dds::get_single_stub<dds::MapService>(consul, opt);

    Void void_type;
    Bool is_empty;
    grpc::ClientContext context;

    grpc::Status status = stub_ptr->Empty(&context, void_type, &is_empty);

    if (!status.ok()) {
        dds::print_grpc_error(status);
    }
    return is_empty.flag();
}

void DDS_Map::remove(const std::string &key) {
    auto stubs_ptrs = dds::get_stubs_by_key<dds::MapService>(consul, opt, key);

    MapKey map_key;
    map_key.set_map_key(key);
    Void void_type;

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->Remove(&context, map_key, &void_type);
        if (!status.ok()) {
            dds::print_grpc_error(status);
        }
    }
}

void DDS_Map::clear() {
    auto stubs_ptrs = dds::get_all_stubs<dds::MapService>(consul, opt);

    Void void_type;

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->Clear(&context, void_type, &void_type);
        if (!status.ok()) {
            dds::print_grpc_error(status);
        }
    }
}
