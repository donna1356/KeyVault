#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
using namespace std;

namespace keyvault {

class KeyValueStore {
public:
    void put(const string& key, const string& value);
    optional<string> get(const string& key) const;
    bool remove(const string& key);
    size_t size() const;

private:
    unordered_map<string, string> data_;
};

}  // namespace keyvault