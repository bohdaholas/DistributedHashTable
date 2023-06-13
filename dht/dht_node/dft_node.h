// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef DDS_CONTROLLER_H
#define DDS_CONTROLLER_H

#include <ppconsul/agent.h>
#include <ppconsul/kv.h>
#include <grpcpp/create_channel.h>
#include "dds.grpc.pb.h"
#include "config_parser.h"

class DHT_Node {
public:
    DHT_Node();

    void set_opt(config_options_t &opt);
    std::string get_id();
    void register_dds_instance();
    void deregister_dds_instance();
    void run();

private:
    config_options_t* opt;
    ppconsul::Consul consul;
    ppconsul::agent::Agent agent;
    std::unordered_map<std::string, int> map;

    class MapServiceImpl : public dds::MapService::Service {
    public:
        MapServiceImpl(const ppconsul::Consul &consul,
                       const config_options_t &opt,
                       std::unordered_map<std::string, int>& map);

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
        std::unordered_map<std::string, int>& map;
    };

    class HealthCheckServiceImpl : public dds::HealthCheck::Service {
        grpc::Status Check(grpc::ServerContext* context,
                           const dds::Void* request,
                           dds::Void* response) override {
            return grpc::Status::OK;
        }
    };

    class RebalancingServiceImpl : public dds::RebalancingService::Service {
    public:
        RebalancingServiceImpl(const ppconsul::Consul &consul,
                               const config_options_t &opt,
                               std::unordered_map<std::string, int>& map);

        grpc::Status Rebalancing(grpc::ServerContext *server_context,
                                 const dds::RebalanceRequest *request,
                                 dds::Map *response) override;

    private:
        const ppconsul::Consul& consul;
        const config_options_t &opt;
        std::unordered_map<std::string, int>& map;
    };

    class Server {
    public:
        Server(const ppconsul::Consul& consul,
               const config_options_t &opt,
               std::unordered_map<std::string, int>& map);
        void run();
    private:
        const ppconsul::Consul &consul;
        const config_options_t &opt;
        std::unordered_map<std::string, int>& map;
    };
};

#endif //DDS_CONTROLLER_H
