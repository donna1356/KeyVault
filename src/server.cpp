#include "keyvault/server.hpp"

#include "keyvault/protocol.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace keyvault {

Server::Server(const std::uint16_t port,
               const std::string& data_file,
               const std::size_t max_capacity,
               std::vector<PeerAddress> peers,
               const std::optional<PeerAddress> seed_peer)
    : port_(port),
      data_file_(data_file),
      store_(max_capacity),
      peers_(std::move(peers)),
      seed_peer_(seed_peer) {
    store_.load_from_disk(data_file_);
    replication_.set_peers(peers_);

    if (seed_peer_.has_value()) {
        if (replication_.sync_from_peer(seed_peer_.value(), store_)) {
            std::cout << "Synced data from peer " << seed_peer_->host << ':'
                      << seed_peer_->port << '\n';
        } else {
            std::cerr << "Warning: failed to sync from peer "
                      << seed_peer_->host << ':' << seed_peer_->port << '\n';
        }
    }
}

Server::~Server() {
    stop();
}

void Server::run() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create server socket");
    }

    const int reuse = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) <
        0) {
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(server_fd_, 16) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to listen on server socket");
    }

    running_ = true;
    expiration_thread_ = std::thread([this]() { expiration_loop(); });

    std::cout << "KeyVault node listening on port " << port_;
    if (!peers_.empty()) {
        std::cout << " (cluster node, " << peers_.size() << " peer";
        if (peers_.size() != 1) {
            std::cout << 's';
        }
        std::cout << ')';
    }
    std::cout << '\n';
    accept_loop();
}

void Server::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    if (expiration_thread_.joinable()) {
        expiration_thread_.join();
    }

    store_.save_to_disk(data_file_);
}

void Server::accept_loop() {
    while (running_) {
        sockaddr_in client_address{};
        socklen_t client_length = sizeof(client_address);

        const int client_fd =
            accept(server_fd_,
                   reinterpret_cast<sockaddr*>(&client_address),
                   &client_length);

        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Accept failed\n";
            }
            continue;
        }

        std::thread(&Server::handle_client, this, client_fd).detach();
    }
}

void Server::handle_client(const int client_fd) {
    while (running_) {
        const std::string line = read_line(client_fd);
        if (line.empty()) {
            break;
        }

        const Command command = parse_command(line);
        std::string response;

        switch (command.type) {
            case Command::Type::Set:
                store_.put(command.key, command.value, command.ttl_seconds);
                replication_.replicate_set(command.key,
                                         command.value,
                                         command.ttl_seconds);
                response = format_ok();
                break;
            case Command::Type::Get: {
                const auto value = store_.get(command.key);
                response = value.has_value() ? format_value(value.value())
                                             : format_not_found();
                break;
            }
            case Command::Type::Delete:
                if (store_.remove(command.key)) {
                    replication_.replicate_delete(command.key);
                    response = format_ok();
                } else {
                    response = format_not_found();
                }
                break;
            case Command::Type::Size:
                response = format_size(store_.size());
                break;
            case Command::Type::Ping:
                response = format_pong();
                break;
            case Command::Type::ReplSet:
                store_.put(command.key, command.value, command.ttl_seconds);
                response = format_ok();
                break;
            case Command::Type::ReplDelete:
                store_.remove(command.key);
                response = format_ok();
                break;
            case Command::Type::ReplSync:
                send_sync_response(client_fd);
                close(client_fd);
                return;
            case Command::Type::Unknown:
            default:
                response = format_error(command.error.empty() ? "Invalid command"
                                                              : command.error);
                break;
        }

        if (!send_all(client_fd, response)) {
            break;
        }
    }

    close(client_fd);
}

void Server::send_sync_response(const int client_fd) {
    for (const StoredEntry& entry : store_.entries()) {
        const std::string line =
            format_repl_set(entry.key,
                            entry.value,
                            remaining_ttl_seconds(entry.expires_at));

        if (!send_all(client_fd, line)) {
            return;
        }
    }

    send_all(client_fd, format_repl_end());
}

std::optional<int> Server::remaining_ttl_seconds(
    const std::optional<std::chrono::system_clock::time_point>& expires_at) const {
    if (!expires_at.has_value()) {
        return std::nullopt;
    }

    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        expires_at.value() - std::chrono::system_clock::now());

    if (remaining.count() <= 0) {
        return std::nullopt;
    }

    return static_cast<int>(remaining.count());
}

void Server::expiration_loop() {
    while (running_) {
        store_.purge_expired();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool Server::send_all(const int socket_fd, const std::string& message) const {
    std::size_t sent = 0;

    while (sent < message.size()) {
        const ssize_t result =
            send(socket_fd, message.data() + sent, message.size() - sent, 0);

        if (result <= 0) {
            return false;
        }

        sent += static_cast<std::size_t>(result);
    }

    return true;
}

std::string Server::read_line(const int socket_fd) const {
    std::string line;
    char buffer[1];

    while (true) {
        const ssize_t bytes_read = recv(socket_fd, buffer, 1, 0);
        if (bytes_read <= 0) {
            return {};
        }

        if (buffer[0] == '\n') {
            break;
        }

        if (buffer[0] != '\r') {
            line += buffer[0];
        }
    }

    return line;
}

}  // namespace keyvault
