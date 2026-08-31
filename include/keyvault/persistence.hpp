#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace keyvault {

struct StoredEntry {
    std::string key;
    std::string value;

    std::optional<std::chrono::system_clock::time_point> expires_at;
};

class Persistence {
public:
    static bool save(
        const std::string& path,
        const std::vector<StoredEntry>& entries
    );

    static bool load(
    const std::string& path,
    std::vector<StoredEntry>& entries
    );
};

} // namespace keyvault