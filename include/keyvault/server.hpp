#pragma once

#include "keyvault/key_value_store.hpp"
#include "keyvault/replication.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace keyvault {

class Server {
public:
    Server(
        std::uint16_t port,
        const std::string& data_file,
        std::size_t capacity,
        std::vector<PeerAddress> peers,
        std::optional<PeerAddress> seed_peer
    );

    ~Server();

    void run();
    void stop();

private:
    void accept_loop();
    void handle_client(int client_fd);
    void send_sync_response(int client_fd);
    void expiration_loop();

    bool send_all(
        int socket_fd,
        const std::string& data
    ) const;

    std::string read_line(int socket_fd) const;

    std::optional<int> remaining_ttl_seconds(
        const std::optional<
            std::chrono::system_clock::time_point
        >& expires_at
    ) const;

    std::uint16_t port_;
    std::string data_file_;

    KeyValueStore store_;

    std::vector<PeerAddress> peers_;
    std::optional<PeerAddress> seed_peer_;

    ReplicationManager replication_;

    std::atomic<bool> running_{false};

    int server_fd_ = -1;

    std::thread expiration_thread_;
};

} // namespace keyvault