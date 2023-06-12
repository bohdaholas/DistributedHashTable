// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef DDS_CONTROLLER_H
#define DDS_CONTROLLER_H

#include <ppconsul/agent.h>
#include <ppconsul/kv.h>
#include "dds.grpc.pb.h"
#include "config_parser.h"

class DDS_Controller {
public:
    DDS_Controller();

    void set_opt(config_options_t &opt);
    std::string get_id();
    void register_dds_instance();
    void deregister_dds_instance();
    void run();

private:
    config_options_t* opt;
    ppconsul::Consul consul;
    ppconsul::agent::Agent agent;

    class HealthCheckServiceImpl : public dds::HealthCheck::Service {
        grpc::Status Check(grpc::ServerContext* context,
                           const dds::HealthCheckRequest* request,
                           dds::HealthCheckResponse* response) override {
            return grpc::Status::OK;
        }
    };

    class MapServiceImpl : public dds::MapService::Service {
    public:
        MapServiceImpl(const ppconsul::Consul &consul,
                       const config_options_t &opt);

        grpc::Status GetValue(grpc::ServerContext *context,
                             const dds::MapKey *request,
                             dds::MapValue *response) override;

        grpc::Status SetValue(grpc::ServerContext *context,
                             const dds::KeyValue *request,
                             dds::Void *response) override;

        grpc::Status Size(grpc::ServerContext *context,
                         const dds::Void *request,
                         dds::Int *response) override;

        grpc::Status Empty(grpc::ServerContext *context,
                          const dds::Void *request,
                          dds::Bool *response) override;

        grpc::Status Remove(grpc::ServerContext *context,
                           const dds::MapKey *request,
                           dds::Void *response) override;

        grpc::Status Clear(grpc::ServerContext *context,
                          const dds::Void *request,
                          dds::Void *response) override;
    private:
        const ppconsul::Consul& consul;
        const config_options_t &opt;
        std::unordered_map<std::string, int> map;
    };

    class Server {
    public:
        Server(const ppconsul::Consul& consul,
               const config_options_t &opt);
        void run();
    private:
        const ppconsul::Consul &consul;
        const config_options_t &opt;
    };
};

#endif //DDS_CONTROLLER_H
