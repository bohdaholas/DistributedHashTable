// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <ppconsul/catalog.h>
#include <thread>
#include <grpcpp/server_builder.h>
#include <grpcpp/security/server_credentials.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include "dds_controller.h"

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

DDS_Controller::DDS_Controller() : agent(consul) {}

void DDS_Controller::set_opt(config_options_t &opt_arg) {
    opt = &opt_arg;
}

std::string DDS_Controller::get_id() {
    return opt->dds_instance_name + "_" + std::to_string(opt->dds_instance_port);
}

void DDS_Controller::register_dds_instance() {
    std::stringstream health_check_endpoint_ss;
    health_check_endpoint_ss << "localhost:" << std::to_string(opt->dds_instance_port);
    agent.registerService(
            ppconsul::agent::kw::id = get_id(),
            ppconsul::agent::kw::name = opt->dds_instance_name,
            ppconsul::agent::kw::port = opt->dds_instance_port,
            ppconsul::agent::kw::check = ppconsul::agent::TcpCheck{health_check_endpoint_ss.str(), std::chrono::seconds(5)}
    );
}

void DDS_Controller::deregister_dds_instance() {
    cout << "Unregistering " << get_id() << endl;
    agent.deregisterService(get_id());
}

void DDS_Controller::run() {
    try {
        opt->dds_instance_port = get_first_usable_port(opt->dds_instance_port);
        register_dds_instance();
        Server server(consul, *opt);
        server.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

DDS_Controller::Server::Server(const ppconsul::Consul& consul,
                               const config_options_t &opt)
        : consul(consul), opt{opt} {}

void DDS_Controller::Server::run() {
    std::string server_address("0.0.0.0:" + std::to_string(opt.dds_instance_port));
    grpc::ServerBuilder builder;
    MapServiceImpl map_service(consul, opt);
    HealthCheckServiceImpl health_check_service;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&map_service);
    builder.RegisterService(&health_check_service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    cout << "--- DDS instance is enabled on port " << opt.dds_instance_port << "!" << endl;
    server->Wait();
}

DDS_Controller::MapServiceImpl::MapServiceImpl(const ppconsul::Consul &consul,
                                               const config_options_t &opt)
        : consul(consul), opt(opt) {}

grpc::Status DDS_Controller::MapServiceImpl::GetValue(grpc::ServerContext *context,
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

grpc::Status DDS_Controller::MapServiceImpl::SetValue(grpc::ServerContext *context,
                                                      const dds::KeyValue *request,
                                                      dds::Void *response) {
    const std::string& key{request->map_key()};
    const int value{request->map_value()};
    cout << "Setting key=value for a map: key = " << key << ", value = " << value << endl;
    map[key] = value;
    return grpc::Status::OK;
}

grpc::Status DDS_Controller::MapServiceImpl::Size(grpc::ServerContext *context,
                                                 const dds::Void *request,
                                                 dds::Int *response) {
    cout << "Checking size of the map: " << map.size() << endl;
    response->set_num(static_cast<int>(map.size()));
    return grpc::Status::OK;
}

grpc::Status DDS_Controller::MapServiceImpl::Empty(grpc::ServerContext *context,
                                                   const dds::Void *request,
                                                   dds::Bool *response) {
    cout << std::boolalpha << "Checking if map is empty: " << map.empty() << endl;
    response->set_flag(map.empty());
    return grpc::Status::OK;
}

grpc::Status DDS_Controller::MapServiceImpl::Remove(grpc::ServerContext *context,
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

grpc::Status DDS_Controller::MapServiceImpl::Clear(grpc::ServerContext *context,
                                                   const dds::Void *request,
                                                   dds::Void *response) {
    cout << "Clear the map" << endl;
    map.clear();
    return grpc::Status::OK;
}
