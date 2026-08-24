#include "keyvault/key_value_store.hpp"

namespace keyvault {

void KeyValueStore::put(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::optional<std::string> KeyValueStore::get(const std::string& key) const {
    const auto item = data_.find(key);

    if (item == data_.end()) {
        return std::nullopt;
    }

    return item->second;
}

bool KeyValueStore::remove(const std::string& key) {
    return data_.erase(key) > 0;
}

std::size_t KeyValueStore::size() const {
    return data_.size();
}

}  // namespace keyvault
