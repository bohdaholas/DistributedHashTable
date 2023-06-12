#include "dds_map.h"

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
    auto stubs_ptrs = get_stubs_by_key(key);
    KeyValue key_value;
    Void void_type;
    key_value.set_map_key(key);
    key_value.set_map_value(value);

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->SetValue(&context, key_value, &void_type);
        if (!status.ok()) {
            print_grpc_error(status);
        }
    }
}

int DDS_Map::get(const std::string &key) {
    auto stubs_ptrs = get_stubs_by_key(key);

    MapKey map_key;
    map_key.set_map_key(key);
    MapValue map_value;

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->GetValue(&context, map_key, &map_value);
        if (!status.ok()) {
            print_grpc_error(status);
        }
    }
    return map_value.map_value();
}

int DDS_Map::size() {
    auto stub_ptr = get_single_stub();

    Void void_type;
    Int keys_num;

    grpc::ClientContext context;
    grpc::Status status = stub_ptr->Size(&context, void_type, &keys_num);
    if (!status.ok()) {
        print_grpc_error(status);
    }
    return keys_num.num();
}

bool DDS_Map::empty() {
    auto stub_ptr = get_single_stub();

    Void void_type;
    Bool is_empty;
    grpc::ClientContext context;

    grpc::Status status = stub_ptr->Empty(&context, void_type, &is_empty);

    if (!status.ok()) {
        print_grpc_error(status);
    }
    return is_empty.flag();
}

void DDS_Map::remove(const std::string &key) {
    auto stubs_ptrs = get_stubs_by_key(key);

    MapKey map_key;
    map_key.set_map_key(key);
    Void void_type;

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->Remove(&context, map_key, &void_type);
        if (!status.ok()) {
            print_grpc_error(status);
        }
    }
}

void DDS_Map::clear() {
    auto stubs_ptrs = get_all_stubs();

    Void void_type;

    for (const auto &stub_ptr: stubs_ptrs) {
        grpc::ClientContext context;
        grpc::Status status = stub_ptr->Clear(&context, void_type, &void_type);
        if (!status.ok()) {
            print_grpc_error(status);
        }
    }
}

std::vector<DDS_Map::node_endpoint_t> DDS_Map::get_all_endpoints() {
    ppconsul::catalog::Catalog catalog(const_cast<ppconsul::Consul &>(consul));
    std::vector<node_endpoint_t> endpoints;
    auto nodes = catalog.service(opt.dds_instance_name);
    for (const auto& node : nodes) {
        int node_port = node.second.port;
        std::string node_endpoint = "localhost:" + std::to_string(node_port);
        endpoints.push_back(node_endpoint);
    }
    return endpoints;
}

std::vector<std::unique_ptr<dds::MapService::Stub>> DDS_Map::get_all_stubs() {
    std::vector<std::unique_ptr<dds::MapService::Stub>> stubs;
    for (const auto &node_endpoint: get_all_endpoints()) {
        auto channel_ptr = grpc::CreateChannel(node_endpoint, grpc::InsecureChannelCredentials());
        stubs.push_back(dds::MapService::NewStub(channel_ptr));
    }
    return stubs;
}

std::unique_ptr<dds::MapService::Stub> DDS_Map::get_single_stub() {
    auto endpoints = get_all_endpoints();
    node_endpoint_t rand_endpoint = get_random_element(endpoints);
    auto channel_ptr = grpc::CreateChannel(rand_endpoint, grpc::InsecureChannelCredentials());
    return dds::MapService::NewStub(channel_ptr);
}

DDS_Map::hash_ring_t DDS_Map::get_hash_ring() {
    ppconsul::catalog::Catalog catalog(const_cast<ppconsul::Consul &>(consul));
    auto nodes = catalog.service(opt.dds_instance_name);
    hash_ring_t cluster_hash_ring;
    cout << "calc ring" << endl;
    for (const auto& node : nodes) {
        int node_port = node.second.port;
        std::string node_endpoint = "localhost:" + std::to_string(node_port);
        cout << "node " << node_port << " ";
        cluster_hash_ring[node_endpoint] = compute_ring_positions(node_port);
    }
    return cluster_hash_ring;
}

void DDS_Map::print_grpc_error(const grpc::Status& status) {
    grpc::StatusCode error_code = status.error_code();
    std::string error_message = status.error_message();
    std::cerr << "Error code: " << error_code << std::endl;
    std::cerr << "Error message: " << error_message << std::endl;
}


