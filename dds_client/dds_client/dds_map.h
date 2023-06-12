#ifndef ALL_MAP_WRAPPER_H
#define ALL_MAP_WRAPPER_H

#include <unordered_map>
#include <string>
#include <grpcpp/grpcpp.h>
#include <ppconsul/catalog.h>
#include <ppconsul/kv.h>
#include <random>
#include "dds.grpc.pb.h"
#include "config_parser.h"

constexpr size_t MAX_NODES_NUM = 360;
constexpr size_t VNODES_PER_NODE = 10;

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
    using node_endpoint_t = std::string;
    using ring_positions_t = std::array<size_t, VNODES_PER_NODE>;
    using hash_ring_t = std::map<node_endpoint_t, ring_positions_t>;

    template<typename T>
    static ring_positions_t compute_ring_positions(T value) {
        std::array<std::string, VNODES_PER_NODE> res_str{};
        std::array<size_t, VNODES_PER_NODE> res{};
        res_str[0] = std::to_string(std::hash<T>{}(value) % MAX_NODES_NUM);
        for (size_t i = 1; i < VNODES_PER_NODE; ++i) {
            res_str[i] = std::to_string(std::hash<std::string>{}(res_str[i - 1]) % MAX_NODES_NUM);
        }
        for (size_t i = 0; i < VNODES_PER_NODE; ++i) {
            res[i] = std::stoull(res_str[i]);
            std::cout << res[i] << " ";
        }
        std::cout << std::endl;
        return res;
    }

    size_t clockwise_angle_diff(size_t angle1, size_t angle2) {
        if (angle1 <= angle2) {
            return angle2 - angle1;
        }
        return 360 - angle1 + angle2;
    }

    template<typename T>
    std::unordered_set<node_endpoint_t> get_closest_endpoints(T key) {
        std::unordered_set<node_endpoint_t> closest_endpoints;
        hash_ring_t hash_ring = get_hash_ring();
        std::cout << "compute key ring positions: ";
        for (const auto &key_ring_pos: compute_ring_positions(key)) {
            node_endpoint_t closest_vnode_endpoint;
            size_t closest_vnode_ring_pos;
            size_t min_angle_diff = UINT32_MAX;
            for (const auto &[node_endpoint, node_ring_positions]: hash_ring) {
                for (const auto &node_ring_pos: node_ring_positions) {
                    if (clockwise_angle_diff(key_ring_pos, node_ring_pos) < min_angle_diff) {
                        min_angle_diff = clockwise_angle_diff(key_ring_pos, node_ring_pos);
                        closest_vnode_endpoint = node_endpoint;
                        closest_vnode_ring_pos = node_ring_pos;
                    }
                }
            }
            closest_endpoints.insert(closest_vnode_endpoint);
            std::cout << "Closest endpoint is " << closest_vnode_endpoint
                      << " with vnode ring pos " << closest_vnode_ring_pos
                      << " with min diff " << min_angle_diff << std::endl;
        }
        return closest_endpoints;
    }

    std::vector<node_endpoint_t> get_all_endpoints();

    std::vector<std::unique_ptr<dds::MapService::Stub>> get_all_stubs();

    template<typename T>
    T get_random_element(const std::vector<T>& vec) {
        std::random_device rd;
        std::mt19937 gen(rd());
        if (!vec.empty()) {
            std::uniform_int_distribution<> dist(0, vec.size() - 1);
            int rand_idx = dist(gen);
            return vec[rand_idx];
        }
    }

    std::unique_ptr<dds::MapService::Stub> get_single_stub();

    template<typename T>
    std::vector<std::unique_ptr<dds::MapService::Stub>> get_stubs_by_key(T key) {
        std::vector<std::unique_ptr<dds::MapService::Stub>> stubs;
        for (const auto &node_endpoint: get_closest_endpoints(key)) {
            auto channel_ptr = grpc::CreateChannel(node_endpoint, grpc::InsecureChannelCredentials());
            stubs.push_back(dds::MapService::NewStub(channel_ptr));
        }
        return stubs;
    }

    hash_ring_t get_hash_ring();

    static void print_grpc_error(const grpc::Status& status);

    const ppconsul::Consul &consul;
    const config_options_t &opt;
};

#endif //ALL_MAP_WRAPPER_H
