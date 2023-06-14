#include <ppconsul/catalog.h>
#include "cluster_management.h"

using std::cout, std::endl;

dds::hash_ring_t dds::get_hash_ring(const ppconsul::Consul &consul,
                                    const config_options_t &opt) {
    ppconsul::catalog::Catalog catalog(const_cast<ppconsul::Consul &>(consul));
    auto nodes = catalog.service(opt.dds_instance_name);
    dds::hash_ring_t cluster_hash_ring;
//    cout << "calc ring" << endl;
    for (const auto& node : nodes) {
        int node_port = node.second.port;
        std::string node_endpoint = "localhost:" + std::to_string(node_port);
//        cout << "node " << node_port << " ";
        cluster_hash_ring[node_endpoint] = dds::compute_ring_positions(node_port);
    }
    return cluster_hash_ring;
}

std::vector<dds::node_endpoint_t> dds::get_all_endpoints(const ppconsul::Consul &consul,
                                                         const config_options_t &opt) {
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

void dds::print_grpc_error(const grpc::Status& status) {
    grpc::StatusCode error_code = status.error_code();
    std::string error_message = status.error_message();
    std::cerr << "Error code: " << error_code << std::endl;
    std::cerr << "Error message: " << error_message << std::endl;
}
