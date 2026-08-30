#include "keyvault/protocol.hpp"

#include <cctype>
#include <sstream>

namespace keyvault {
namespace {

std::string to_upper(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
    }

    return text;
}

bool has_extra_argument(std::istringstream& input) {
    std::string extra;
    return static_cast<bool>(input >> extra);
}

}  // namespace

Command parse_command(const std::string& line) {
    Command command;
    std::istringstream input(line);
    std::string verb;

    input >> verb;
    if (verb.empty()) {
        command.error = "Empty command";
        return command;
    }

    verb = to_upper(verb);

    if (verb == "SET" || verb == "PUT") {
        command.type = Command::Type::Set;

        if (!(input >> command.key)) {
            command.error = "Usage: SET <key> <value> [ttl_seconds]";
            command.type = Command::Type::Unknown;
            return command;
        }

        std::getline(input >> std::ws, command.value);
        if (command.value.empty()) {
            command.error = "Usage: SET <key> <value> [ttl_seconds]";
            command.type = Command::Type::Unknown;
            return command;
        }

        std::string ttl_token;
        const std::size_t last_space = command.value.find_last_of(' ');
        if (last_space != std::string::npos) {
            ttl_token = command.value.substr(last_space + 1);
            bool digits_only = !ttl_token.empty();
            for (const char character : ttl_token) {
                if (!std::isdigit(static_cast<unsigned char>(character))) {
                    digits_only = false;
                    break;
                }
            }

            if (digits_only) {
                command.ttl_seconds = std::stoi(ttl_token);
                command.value = command.value.substr(0, last_space);
                if (command.value.empty()) {
                    command.error = "Usage: SET <key> <value> [ttl_seconds]";
                    command.type = Command::Type::Unknown;
                }
            }
        }

        return command;
    }

    if (verb == "GET") {
        command.type = Command::Type::Get;

        if (!(input >> command.key) || has_extra_argument(input)) {
            command.error = "Usage: GET <key>";
            command.type = Command::Type::Unknown;
        }

        return command;
    }

    if (verb == "DELETE") {
        command.type = Command::Type::Delete;

        if (!(input >> command.key) || has_extra_argument(input)) {
            command.error = "Usage: DELETE <key>";
            command.type = Command::Type::Unknown;
        }

        return command;
    }

    if (verb == "SIZE") {
        command.type = Command::Type::Size;

        if (has_extra_argument(input)) {
            command.error = "Usage: SIZE";
            command.type = Command::Type::Unknown;
        }

        return command;
    }

    if (verb == "PING") {
        command.type = Command::Type::Ping;

        if (has_extra_argument(input)) {
            command.error = "Usage: PING";
            command.type = Command::Type::Unknown;
        }

        return command;
    }

    if (verb == "REPL") {
        std::string repl_verb;
        input >> repl_verb;
        repl_verb = to_upper(repl_verb);

        if (repl_verb == "SET") {
            command.type = Command::Type::ReplSet;

            if (!(input >> command.key)) {
                command.error = "Usage: REPL SET <key> <value> [ttl_seconds]";
                command.type = Command::Type::Unknown;
                return command;
            }

            std::getline(input >> std::ws, command.value);
            if (command.value.empty()) {
                command.error = "Usage: REPL SET <key> <value> [ttl_seconds]";
                command.type = Command::Type::Unknown;
                return command;
            }

            const std::size_t last_space = command.value.find_last_of(' ');
            if (last_space != std::string::npos) {
                const std::string ttl_token = command.value.substr(last_space + 1);
                bool digits_only = !ttl_token.empty();
                for (const char character : ttl_token) {
                    if (!std::isdigit(static_cast<unsigned char>(character))) {
                        digits_only = false;
                        break;
                    }
                }

                if (digits_only) {
                    command.ttl_seconds = std::stoi(ttl_token);
                    command.value = command.value.substr(0, last_space);
                    if (command.value.empty()) {
                        command.error =
                            "Usage: REPL SET <key> <value> [ttl_seconds]";
                        command.type = Command::Type::Unknown;
                    }
                }
            }

            return command;
        }

        if (repl_verb == "DELETE") {
            command.type = Command::Type::ReplDelete;

            if (!(input >> command.key) || has_extra_argument(input)) {
                command.error = "Usage: REPL DELETE <key>";
                command.type = Command::Type::Unknown;
            }

            return command;
        }

        if (repl_verb == "SYNC") {
            command.type = Command::Type::ReplSync;

            if (has_extra_argument(input)) {
                command.error = "Usage: REPL SYNC";
                command.type = Command::Type::Unknown;
            }

            return command;
        }

        command.error = "Unknown replication command";
        return command;
    }

    command.error = "Unknown command";
    return command;
}

std::string format_ok() { return "OK\n"; }

std::string format_value(const std::string& value) {
    return "VALUE " + value + '\n';
}

std::string format_not_found() { return "NOT_FOUND\n"; }

std::string format_size(const std::size_t count) {
    return std::to_string(count) + '\n';
}

std::string format_error(const std::string& message) {
    return "ERROR " + message + '\n';
}

std::string format_pong() { return "PONG\n"; }

std::string format_repl_set(const std::string& key,
                            const std::string& value,
                            const std::optional<int> ttl_seconds) {
    if (ttl_seconds.has_value()) {
        return "REPL SET " + key + ' ' + value + ' ' +
               std::to_string(ttl_seconds.value()) + '\n';
    }

    return "REPL SET " + key + ' ' + value + '\n';
}

std::string format_repl_delete(const std::string& key) {
    return "REPL DELETE " + key + '\n';
}

std::string format_repl_end() { return "REPL END\n"; }

}  // namespace keyvault
