#include "keyvault/server.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>

namespace {

keyvault::Server* g_server = nullptr;

void handle_shutdown_signal(const int) {
    if (g_server != nullptr) {
        g_server->stop();
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::uint16_t port = 7379;
    std::string data_file = "keyvault.dat";
    std::size_t max_capacity = 1000;
    std::string peers_argument;
    std::string seed_peer_argument;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--port" && index + 1 < argc) {
            port = static_cast<std::uint16_t>(std::stoi(argv[++index]));
        } else if (argument == "--data-file" && index + 1 < argc) {
            data_file = argv[++index];
        } else if (argument == "--capacity" && index + 1 < argc) {
            max_capacity = static_cast<std::size_t>(std::stoul(argv[++index]));
        } else if (argument == "--peers" && index + 1 < argc) {
            peers_argument = argv[++index];
        } else if (argument == "--seed-peer" && index + 1 < argc) {
            seed_peer_argument = argv[++index];
        } else if (argument == "--help") {
            std::cout << "Usage: keyvault-server [--port PORT] "
                         "[--data-file PATH] [--capacity N] "
                         "[--peers host:port,host:port] "
                         "[--seed-peer host:port]\n";
            return 0;
        }
    }

    const std::vector<keyvault::PeerAddress> peers =
        keyvault::parse_peer_list(peers_argument);
    std::optional<keyvault::PeerAddress> seed_peer;
    if (!seed_peer_argument.empty()) {
        seed_peer = keyvault::parse_peer_address(seed_peer_argument);
    }

    try {
        keyvault::Server server(port, data_file, max_capacity, peers, seed_peer);
        g_server = &server;

        std::signal(SIGINT, handle_shutdown_signal);
        std::signal(SIGTERM, handle_shutdown_signal);

        server.run();
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
