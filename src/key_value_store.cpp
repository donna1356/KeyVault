#include "keyvault/key_value_store.hpp"

namespace keyvault {

void KeyValueStore::put(const string& key, const string& value) {
    unique_lock<shared_mutex> lock(mutex_); //unique lock because it is a write operation and we want to prevent other threads from reading or writing while this operation is in progress.
    data_[key] = value;
}

optional<string> KeyValueStore::get(const string& key) const {
    shared_lock<shared_mutex> lock(mutex_);
    const auto item = data_.find(key);

    if (item == data_.end()) {
        return nullopt;
    }

    return item->second;
}

bool KeyValueStore::remove(const string& key) {
    unique_lock<shared_mutex> lock(mutex_); //write operation
    return data_.erase(key) > 0;
}

size_t KeyValueStore::size() const {
    shared_lock<shared_mutex> lock(mutex_);
    return data_.size();
}



}  // namespace keyvault
