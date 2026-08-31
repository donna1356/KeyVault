#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace keyvault {

struct Command {
    enum class Type {
        Unknown,
        Set,
        Get,
        Delete,
        Size,
        Ping,
        ReplSet,
        ReplDelete,
        ReplSync
    };

    Type type = Type::Unknown;

    std::string key;
    std::string value;

    std::optional<int> ttl_seconds;

    std::string error;
};

Command parse_command(const std::string& line);

std::string format_ok();
std::string format_value(const std::string& value);
std::string format_not_found();
std::string format_size(std::size_t count);
std::string format_error(const std::string& message);
std::string format_pong();

std::string format_repl_set(
    const std::string& key,
    const std::string& value,
    std::optional<int> ttl_seconds
);

std::string format_repl_delete(const std::string& key);
std::string format_repl_end();

} // namespace keyvault