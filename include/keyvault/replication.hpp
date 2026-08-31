#pragma once

#include "keyvault/key_value_store.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace keyvault {

struct PeerAddress {
    std::string host;
    std::uint16_t port = 0;
};

PeerAddress parse_peer_address(const std::string& peer);

std::vector<PeerAddress> parse_peer_list(
    const std::string& peers
);

class ReplicationManager {
public:
    void set_peers(std::vector<PeerAddress> peers);

    void replicate_set(
        const std::string& key,
        const std::string& value,
        std::optional<int> ttl_seconds
    );

    void replicate_delete(
        const std::string& key
    );

    bool sync_from_peer(
        const PeerAddress& peer,
        KeyValueStore& store
    );

private:
    std::vector<PeerAddress> peers_;
};

} // namespace keyvault