#include "keyvault/persistence.hpp"

#include <fstream>
#include <sstream>

namespace keyvault {
namespace {

constexpr const char* kSnapshotHeader = "# KeyVault snapshot v1";

std::string escape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (const char character : text) {
        if (character == '\\') {
            escaped += "\\\\";
        } else if (character == '\n') {
            escaped += "\\n";
        } else if (character == '\r') {
            escaped += "\\r";
        } else if (character == '\t') {
            escaped += "\\t";
        } else {
            escaped += character;
        }
    }

    return escaped;
}

std::string unescape(const std::string& text) {
    std::string unescaped;
    unescaped.reserve(text.size());

    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\\' && index + 1 < text.size()) {
            const char next = text[++index];

            if (next == 'n') {
                unescaped += '\n';
            } else if (next == 'r') {
                unescaped += '\r';
            } else if (next == 't') {
                unescaped += '\t';
            } else {
                unescaped += next;
            }
        } else {
            unescaped += text[index];
        }
    }

    return unescaped;
}

}  // namespace

bool Persistence::save(
    const std::string& path,
    const std::vector<StoredEntry>& entries
) {
    std::ofstream output(path, std::ios::trunc);

    if (!output) {
        return false;
    }

    output << kSnapshotHeader << '\n';

    for (const StoredEntry& entry : entries) {
        long long expiry_ms = 0;

        if (entry.expires_at.has_value()) {
            expiry_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    entry.expires_at->time_since_epoch()
                ).count();
        }

        output << escape(entry.key)
               << '\t'
               << escape(entry.value)
               << '\t'
               << expiry_ms
               << '\n';
    }

    return static_cast<bool>(output);
}

bool Persistence::load(
    const std::string& path,
    std::vector<StoredEntry>& entries
) {
    std::ifstream input(path);

    if (!input.is_open()) {
        return false;
    }

    std::string header;

    if (!std::getline(input, header)) {
        return false;
    }

    if (header != kSnapshotHeader) {
        return false;
    }

    std::vector<StoredEntry> loaded_entries;

    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);

        std::string encoded_key;
        std::string encoded_value;
        std::string expiry_text;

        if (!std::getline(stream, encoded_key, '\t')) {
            continue;
        }

        if (!std::getline(stream, encoded_value, '\t')) {
            continue;
        }

        if (!std::getline(stream, expiry_text)) {
            continue;
        }

        StoredEntry entry;

        entry.key = unescape(encoded_key);
        entry.value = unescape(encoded_value);

        try {
            const long long expiry_ms = std::stoll(expiry_text);

            if (expiry_ms > 0) {
                entry.expires_at =
                    std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(expiry_ms)
                    );
            }
        } catch (...) {
            continue;
        }

        loaded_entries.push_back(std::move(entry));
    }

    entries = std::move(loaded_entries);

    return true;
}

}  // namespace keyvault