#include "keyvault/key_value_store.hpp"

namespace keyvault {

KeyValueStore::KeyValueStore(const std::size_t max_capacity)
    : cache_(max_capacity) {}

void KeyValueStore::put(const std::string& key,
                       const std::string& value,
                       std::optional<int> ttl_seconds) {
    Entry entry;
    entry.value = value;

    if (ttl_seconds.has_value() && ttl_seconds.value() > 0) {
        entry.expires_at =
            std::chrono::system_clock::now() +
            std::chrono::seconds(ttl_seconds.value());
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);
    cache_.put(key, std::move(entry));
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    const auto entry = cache_.get(key);
    if (!entry.has_value()) {
        return std::nullopt;
    }

    if (is_expired(entry.value())) {
        cache_.remove(key);
        return std::nullopt;
    }

    return entry->value;
}

bool KeyValueStore::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return cache_.remove(key);
}

std::size_t KeyValueStore::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cache_.size();
}

void KeyValueStore::purge_expired() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    for (const auto& item : cache_.entries()) {
        if (is_expired(item.second)) {
            cache_.remove(item.first);
        }
    }
}

bool KeyValueStore::save_to_disk(const std::string& path) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<StoredEntry> entries;
    entries.reserve(cache_.size());

    for (const auto& item : cache_.entries()) {
        if (is_expired(item.second)) {
            continue;
        }

        StoredEntry stored;
        stored.key = item.first;
        stored.value = item.second.value;
        stored.expires_at = item.second.expires_at;
        entries.push_back(std::move(stored));
    }

    lock.unlock();
    return Persistence::save(path, entries);
}

bool KeyValueStore::load_from_disk(const std::string& path) {
    const std::vector<StoredEntry> entries = Persistence::load(path);

    std::unique_lock<std::shared_mutex> lock(mutex_);
    cache_.clear();

    const auto now = std::chrono::system_clock::now();
    for (const StoredEntry& stored : entries) {
        if (stored.expires_at.has_value() && stored.expires_at.value() <= now) {
            continue;
        }

        Entry entry;
        entry.value = stored.value;
        entry.expires_at = stored.expires_at;
        cache_.put(stored.key, std::move(entry));
    }

    return true;
}

std::vector<StoredEntry> KeyValueStore::entries() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<StoredEntry> result;
    result.reserve(cache_.size());

    for (const auto& item : cache_.entries()) {
        if (is_expired(item.second)) {
            continue;
        }

        StoredEntry stored;
        stored.key = item.first;
        stored.value = item.second.value;
        stored.expires_at = item.second.expires_at;
        result.push_back(std::move(stored));
    }

    return result;
}

bool KeyValueStore::is_expired(const Entry& entry) const {
    return entry.expires_at.has_value() &&
           entry.expires_at.value() <= std::chrono::system_clock::now();
}

}  // namespace keyvault
