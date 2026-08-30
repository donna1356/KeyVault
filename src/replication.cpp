#include "keyvault/replication.hpp"

#include "keyvault/protocol.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

namespace keyvault {
namespace {

int connect_to_peer(const PeerAddress& peer) {
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return -1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(peer.port);

    if (inet_pton(AF_INET, peer.host.c_str(), &address.sin_addr) <= 0) {
        close(socket_fd);
        return -1;
    }

    if (connect(socket_fd,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) < 0) {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

bool send_all(const int socket_fd, const std::string& message) {
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

std::string read_line(const int socket_fd) {
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

}  // namespace

PeerAddress parse_peer_address(const std::string& peer) {
    PeerAddress address;

    const std::size_t separator = peer.rfind(':');
    if (separator == std::string::npos || separator == peer.size() - 1) {
        return address;
    }

    address.host = peer.substr(0, separator);
    address.port = static_cast<std::uint16_t>(std::stoi(peer.substr(separator + 1)));
    return address;
}

std::vector<PeerAddress> parse_peer_list(const std::string& peers) {
    std::vector<PeerAddress> result;
    std::stringstream stream(peers);
    std::string peer;

    while (std::getline(stream, peer, ',')) {
        if (peer.empty()) {
            continue;
        }

        const PeerAddress address = parse_peer_address(peer);
        if (!address.host.empty() && address.port > 0) {
            result.push_back(address);
        }
    }

    return result;
}

void ReplicationManager::set_peers(std::vector<PeerAddress> peers) {
    peers_ = std::move(peers);
}

void ReplicationManager::replicate_set(const std::string& key,
                                       const std::string& value,
                                       const std::optional<int> ttl_seconds) {
    const std::string command = format_repl_set(key, value, ttl_seconds);

    for (const PeerAddress& peer : peers_) {
        std::thread([peer, command]() {
            const int socket_fd = connect_to_peer(peer);
            if (socket_fd < 0) {
                return;
            }

            send_all(socket_fd, command);
            close(socket_fd);
        }).detach();
    }
}

void ReplicationManager::replicate_delete(const std::string& key) {
    const std::string command = format_repl_delete(key);

    for (const PeerAddress& peer : peers_) {
        std::thread([peer, command]() {
            const int socket_fd = connect_to_peer(peer);
            if (socket_fd < 0) {
                return;
            }

            send_all(socket_fd, command);
            close(socket_fd);
        }).detach();
    }
}

bool ReplicationManager::sync_from_peer(const PeerAddress& peer,
                                        KeyValueStore& store) {
    const int socket_fd = connect_to_peer(peer);
    if (socket_fd < 0) {
        return false;
    }

    if (!send_all(socket_fd, "REPL SYNC\n")) {
        close(socket_fd);
        return false;
    }

    while (true) {
        const std::string line = read_line(socket_fd);
        if (line.empty()) {
            close(socket_fd);
            return false;
        }

        if (line == "REPL END") {
            break;
        }

        const Command command = parse_command(line);
        if (command.type == Command::Type::ReplSet) {
            store.put(command.key, command.value, command.ttl_seconds);
        }
    }

    close(socket_fd);
    return true;
}

}  // namespace keyvault
