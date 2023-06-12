// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <thread>
#include "dds_client.h"
#include "config_parser.h"

using std::cout, std::cerr, std::endl;

void handle_sigterm() {
    cout << "--- DDS instance is disabled!" << endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
//    std::string filename;
//    if (argc == 1) {
//        filename = "../config.cfg";
//    } else if (argc == 2) {
//        filename = argv[1];
//    } else {
//        cerr << "Wrong number of arguments" << endl;
//        exit(EXIT_FAILURE);
//    }

//    config_options_t opt{filename};
    config_options_t opt{"../../config.cfg"};
    auto &dds_client = DDS_Client::get_instance(opt);
    auto map = dds_client.get_map();
    for (size_t i = 0; i < 3; ++i) {
        std::string key = "word" + std::to_string(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        cout << "Put " << i << endl;
        map.put(key, (int) i);
    }
    cout << std::boolalpha << "Map empty:" << map.empty() << endl;
    cout << "Map size:" << map.size() << endl;
    map.remove("current 0 value");
    cout << "Map size:" << map.size() << endl;
    for (size_t i = 1; i < 3; ++i) {
        std::string key = "word" + std::to_string(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "Get " << map.get(key) << endl;
    }
    cout << "Map clear" << endl;
    map.clear();
    cout << std::boolalpha << "Map empty:" << map.empty() << endl;
    cout << "Map size:" << map.size() << endl;
}
