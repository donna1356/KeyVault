#pragma once

#include "keyvault/lru_cache.hpp"
#include "keyvault/persistence.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <string>

namespace keyvault {

struct Entry {
    std::string value;
    std::optional<std::chrono::system_clock::time_point> expires_at;
};

class KeyValueStore {
public:
    explicit KeyValueStore(std::size_t max_capacity = 1000);

    void put(const std::string& key,
             const std::string& value,
             std::optional<int> ttl_seconds = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    bool remove(const std::string& key);
    std::size_t size() const;
    void purge_expired();

    bool save_to_disk(const std::string& path) const;
    bool load_from_disk(const std::string& path);
    std::vector<StoredEntry> entries() const;

private:
    bool is_expired(const Entry& entry) const;

    mutable std::shared_mutex mutex_;
    LRUCache<std::string, Entry> cache_;
};

}  // namespace keyvault
