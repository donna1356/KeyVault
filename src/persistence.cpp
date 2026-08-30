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

bool Persistence::save(const std::string& path,
                       const std::vector<StoredEntry>& entries) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        return false;
    }

    output << kSnapshotHeader << '\n';

    for (const StoredEntry& entry : entries) {
        const auto expiry_ms =
            entry.expires_at.has_value()
                ? std::chrono::duration_cast<std::chrono::milliseconds>(
                      entry.expires_at->time_since_epoch())
                      .count()
                : 0;

        output << escape(entry.key) << '\t' << escape(entry.value) << '\t'
               << expiry_ms << '\n';
    }

    return static_cast<bool>(output);
}

std::vector<StoredEntry> Persistence::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return {};
    }

    std::string header;
    if (!std::getline(input, header) || header != kSnapshotHeader) {
        return {};
    }

    std::vector<StoredEntry> entries;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string encoded_key;
        std::string encoded_value;
        long long expiry_ms = 0;

        if (!std::getline(stream, encoded_key, '\t') ||
            !std::getline(stream, encoded_value, '\t') ||
            !(stream >> expiry_ms)) {
            continue;
        }

        StoredEntry entry;
        entry.key = unescape(encoded_key);
        entry.value = unescape(encoded_value);

        if (expiry_ms > 0) {
            entry.expires_at = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(expiry_ms));
        }

        entries.push_back(std::move(entry));
    }

    return entries;
}

}  // namespace keyvault
