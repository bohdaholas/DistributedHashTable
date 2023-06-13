// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <csignal>
#include "dft_node.h"
#include "config_parser.h"

using std::cout, std::cerr, std::endl;

static DHT_Node dds_controller;

void handle_sigterm() {
    dds_controller.deregister_dds_instance();
    cout << "--- DDS instance is disabled!" << endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    struct sigaction sa{};
    std::string filename;
    if (argc == 1) {
        filename = "../config.cfg";
    } else if (argc == 2) {
        filename = argv[1];
    } else {
        cerr << "Wrong number of arguments" << endl;
        exit(EXIT_FAILURE);
    }

    config_options_t opt{filename};

    sa.sa_handler = reinterpret_cast<__sighandler_t>(&handle_sigterm);
    sigaction(SIGTERM, &sa, nullptr);

    dds_controller.set_opt(opt);
    dds_controller.run();
}
