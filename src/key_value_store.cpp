#include "keyvault/key_value_store.hpp"

namespace keyvault {

void KeyValueStore::put(const string& key, const string& value) {
    data_[key] = value;
}

optional<string> KeyValueStore::get(const string& key) const {
    const auto item = data_.find(key);

    if (item == data_.end()) {
        return nullopt;
    }

    return item->second;
}

bool KeyValueStore::remove(const string& key) {
    return data_.erase(key) > 0;
}

size_t KeyValueStore::size() const {
    return data_.size();
}

}  // namespace keyvault
