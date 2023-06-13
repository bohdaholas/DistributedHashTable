// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <ppconsul/catalog.h>
#include <thread>
#include <grpcpp/server_builder.h>
#include <grpcpp/security/server_credentials.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <ranges>
#include "dft_node.h"
#include "cluster_management.h"

using dds::MapKey;
using dds::MapValue;
using dds::KeyValue;
using dds::Void;
using dds::Int;
using dds::Bool;

using std::cout, std::endl;
using namespace boost::asio;
using namespace boost::asio::ip;

static bool is_port_busy(int port) {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor(io_service);
    boost::system::error_code ec;

    acceptor.open(boost::asio::ip::tcp::v4(), ec);
    if (ec) {
        return true; // error opening the acceptor means port is busy
    }

    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port), ec);
    if (ec) {
        return true; // error binding to the port means port is busy
    }

    acceptor.close();
    return false;
}

static int get_first_usable_port(int start_port) {
    int test_port = start_port;
    while (is_port_busy(test_port))
        test_port++;
    return test_port;
}

DHT_Node::DHT_Node() : agent(consul) {}

void DHT_Node::set_opt(config_options_t &opt_arg) {
    opt = &opt_arg;
}

std::string DHT_Node::get_id() {
    return opt->dds_instance_name + "_" + std::to_string(opt->dds_instance_port);
}

void DHT_Node::register_dds_instance() {
    std::stringstream health_check_endpoint_ss;
    health_check_endpoint_ss << "localhost:" << std::to_string(opt->dds_instance_port);
    agent.registerService(
            ppconsul::agent::kw::id = get_id(),
            ppconsul::agent::kw::name = opt->dds_instance_name,
            ppconsul::agent::kw::port = opt->dds_instance_port,
            ppconsul::agent::kw::check = ppconsul::agent::TcpCheck{health_check_endpoint_ss.str(), std::chrono::seconds(5)}
    );
}

void DHT_Node::deregister_dds_instance() {
    cout << "Unregistering " << get_id() << endl;
    agent.deregisterService(get_id());
}

void DHT_Node::run() {
    try {
        opt->dds_instance_port = get_first_usable_port(opt->dds_instance_port);
        register_dds_instance();
        Server server(consul, *opt, map);
        server.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

DHT_Node::Server::Server(const ppconsul::Consul& consul,
                         const config_options_t &opt,
                         std::unordered_map<std::string, int>& map)
        : consul(consul), opt{opt}, map(map) {}

void DHT_Node::Server::run() {
    std::string server_address("0.0.0.0:" + std::to_string(opt.dds_instance_port));
    grpc::ServerBuilder builder;

    MapServiceImpl map_service(consul, opt, map);
    HealthCheckServiceImpl health_check_service;
    RebalancingServiceImpl rebalancing_service(consul, opt, map);

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&map_service);
    builder.RegisterService(&health_check_service);
    builder.RegisterService(&rebalancing_service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    cout << "--- DDS instance is enabled on port " << opt.dds_instance_port << "!" << endl;
    server->Wait();
}

DHT_Node::MapServiceImpl::MapServiceImpl(const ppconsul::Consul &consul,
                                         const config_options_t &opt,
                                         std::unordered_map<std::string, int>& map)
        : consul(consul), opt(opt), map(map) {}

grpc::Status
DHT_Node::MapServiceImpl::GetValue(grpc::ServerContext *context,
                                   const dds::MapKey *request,
                                   dds::MapValue *response) {
    cout << "Getting value from a map: ";
    const std::string& key{request->map_key()};
    if (map.contains(key)) {
        cout << map[key] << endl;
        response->set_map_value(map[key]);
        return grpc::Status::OK;
    } else {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Key not found in the map");
    }
}

grpc::Status DHT_Node::MapServiceImpl::SetValue(grpc::ServerContext *context,
                                                const dds::KeyValue *request,
                                                dds::Void *response) {
    const std::string& key{request->map_key()};
    const int value{request->map_value()};
    cout << "Setting key=value for a map: key = " << key << ", value = " << value << endl;
    map[key] = value;
    return grpc::Status::OK;
}

grpc::Status DHT_Node::MapServiceImpl::Size(grpc::ServerContext *context,
                                            const dds::Void *request,
                                            dds::Int *response) {
    cout << "Checking size of the map: " << map.size() << endl;
    response->set_num(static_cast<int>(map.size()));
    return grpc::Status::OK;
}

grpc::Status DHT_Node::MapServiceImpl::Empty(grpc::ServerContext *context,
                                             const dds::Void *request,
                                             dds::Bool *response) {
    cout << std::boolalpha << "Checking if map is empty: " << map.empty() << endl;
    response->set_flag(map.empty());
    return grpc::Status::OK;
}

grpc::Status DHT_Node::MapServiceImpl::Remove(grpc::ServerContext *context,
                                              const dds::MapKey *request,
                                              dds::Void *response) {
    const std::string& key{request->map_key()};
    cout << "Removing key/value pair from a map: ";
    if (map.contains(key)) {
        cout << "key = " << key << ", value = " << map[key] << endl;
        map.erase(key);
        return grpc::Status::OK;
    } else {
        cout << "key not found" << endl;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Key not found in the map");
    }
}

grpc::Status DHT_Node::MapServiceImpl::Clear(grpc::ServerContext *context,
                                             const dds::Void *request,
                                             dds::Void *response) {
    cout << "Clear the map" << endl;
    map.clear();
    return grpc::Status::OK;
}

DHT_Node::RebalancingServiceImpl::RebalancingServiceImpl(const ppconsul::Consul &consul,
                                                         const config_options_t &opt,
                                                         std::unordered_map<std::string, int>& map)
                                                               : consul(consul), opt(opt), map(map) {}

grpc::Status DHT_Node::RebalancingServiceImpl::Rebalancing(grpc::ServerContext *server_context,
                                                           const dds::RebalanceRequest *request,
                                                           dds::Map *response) {
    std::vector<std::string> keysToTransfer;
    for (const auto& [key, value] : map) {
        auto hashes = dds::compute_ring_positions(key);
        bool transferNeeded = std::ranges::any_of(hashes, [request](size_t hash) {
            return request->prev_vnode_hash() < hash && hash <= request->new_vnode_hash();
        });
        if (transferNeeded) {
            response->mutable_map()->insert({key, value});
            keysToTransfer.push_back(key);
        }
    }
    for (const auto& key : keysToTransfer) {
        map.erase(key);
    }
    return grpc::Status::OK;
}



