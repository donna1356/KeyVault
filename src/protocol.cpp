#include "keyvault/protocol.hpp"

#include <cctype>
#include <limits>
#include <sstream>

namespace keyvault {
namespace {

std::string to_upper(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character))
        );
    }

    return text;
}

bool has_extra_argument(std::istringstream& input) {
    std::string extra;
    return static_cast<bool>(input >> extra);
}

bool parse_ttl_suffix(
    std::string& value,
    std::optional<int>& ttl_seconds
) {
    // TTL syntax:
    // SET key value EX 60
    //
    // We specifically look for " EX " so a normal numeric
    // value such as "21" is not accidentally treated as TTL.

    const std::size_t ex_position = value.rfind(" EX ");

    if (ex_position == std::string::npos) {
        return true;
    }

    const std::string actual_value =
        value.substr(0, ex_position);

    const std::string ttl_text =
        value.substr(ex_position + 4);

    if (actual_value.empty() || ttl_text.empty()) {
        return false;
    }

    try {
        std::size_t processed = 0;

        const long long ttl =
            std::stoll(ttl_text, &processed);

        // Reject things such as:
        // EX 10abc
        if (processed != ttl_text.size()) {
            return false;
        }

        // TTL must be positive and fit inside int.
        if (ttl <= 0 ||
            ttl > std::numeric_limits<int>::max()) {
            return false;
        }

        ttl_seconds = static_cast<int>(ttl);
        value = actual_value;

        return true;

    } catch (...) {
        return false;
    }
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


    // =========================================================
    // SET / PUT
    // =========================================================

    if (verb == "SET" || verb == "PUT") {
        command.type = Command::Type::Set;

        if (!(input >> command.key)) {
            command.error =
                "Usage: SET <key> <value> [EX <ttl_seconds>]";

            command.type = Command::Type::Unknown;

            return command;
        }

        std::getline(
            input >> std::ws,
            command.value
        );

        if (command.value.empty()) {
            command.error =
                "Usage: SET <key> <value> [EX <ttl_seconds>]";

            command.type = Command::Type::Unknown;

            return command;
        }

        if (!parse_ttl_suffix(
                command.value,
                command.ttl_seconds)) {

            command.error =
                "TTL must be a positive integer";

            command.type =
                Command::Type::Unknown;

            return command;
        }

        return command;
    }


    // =========================================================
    // GET
    // =========================================================

    if (verb == "GET") {
        command.type = Command::Type::Get;

        if (!(input >> command.key) ||
            has_extra_argument(input)) {

            command.error =
                "Usage: GET <key>";

            command.type =
                Command::Type::Unknown;
        }

        return command;
    }


    // =========================================================
    // DELETE
    // =========================================================

    if (verb == "DELETE") {
        command.type =
            Command::Type::Delete;

        if (!(input >> command.key) ||
            has_extra_argument(input)) {

            command.error =
                "Usage: DELETE <key>";

            command.type =
                Command::Type::Unknown;
        }

        return command;
    }


    // =========================================================
    // SIZE
    // =========================================================

    if (verb == "SIZE") {
        command.type =
            Command::Type::Size;

        if (has_extra_argument(input)) {
            command.error =
                "Usage: SIZE";

            command.type =
                Command::Type::Unknown;
        }

        return command;
    }


    // =========================================================
    // PING
    // =========================================================

    if (verb == "PING") {
        command.type =
            Command::Type::Ping;

        if (has_extra_argument(input)) {
            command.error =
                "Usage: PING";

            command.type =
                Command::Type::Unknown;
        }

        return command;
    }


    // =========================================================
    // REPLICATION COMMANDS
    // =========================================================

    if (verb == "REPL") {
        std::string repl_verb;

        input >> repl_verb;

        repl_verb =
            to_upper(repl_verb);


        // -----------------------------------------------------
        // REPL SET
        // -----------------------------------------------------

        if (repl_verb == "SET") {
            command.type =
                Command::Type::ReplSet;

            if (!(input >> command.key)) {
                command.error =
                    "Usage: REPL SET <key> <value> "
                    "[EX <ttl_seconds>]";

                command.type =
                    Command::Type::Unknown;

                return command;
            }

            std::getline(
                input >> std::ws,
                command.value
            );

            if (command.value.empty()) {
                command.error =
                    "Usage: REPL SET <key> <value> "
                    "[EX <ttl_seconds>]";

                command.type =
                    Command::Type::Unknown;

                return command;
            }

            if (!parse_ttl_suffix(
                    command.value,
                    command.ttl_seconds)) {

                command.error =
                    "TTL must be a positive integer";

                command.type =
                    Command::Type::Unknown;

                return command;
            }

            return command;
        }


        // -----------------------------------------------------
        // REPL DELETE
        // -----------------------------------------------------

        if (repl_verb == "DELETE") {
            command.type =
                Command::Type::ReplDelete;

            if (!(input >> command.key) ||
                has_extra_argument(input)) {

                command.error =
                    "Usage: REPL DELETE <key>";

                command.type =
                    Command::Type::Unknown;
            }

            return command;
        }


        // -----------------------------------------------------
        // REPL SYNC
        // -----------------------------------------------------

        if (repl_verb == "SYNC") {
            command.type =
                Command::Type::ReplSync;

            if (has_extra_argument(input)) {
                command.error =
                    "Usage: REPL SYNC";

                command.type =
                    Command::Type::Unknown;
            }

            return command;
        }


        command.error =
            "Unknown replication command";

        return command;
    }


    // =========================================================
    // UNKNOWN COMMAND
    // =========================================================

    command.error =
        "Unknown command";

    return command;
}


// =============================================================
// RESPONSE FORMATTERS
// =============================================================

std::string format_ok() {
    return "OK\n";
}


std::string format_value(
    const std::string& value
) {
    return "VALUE " + value + '\n';
}


std::string format_not_found() {
    return "NOT_FOUND\n";
}


std::string format_size(
    const std::size_t count
) {
    return std::to_string(count) + '\n';
}


std::string format_error(
    const std::string& message
) {
    return "ERROR " + message + '\n';
}


std::string format_pong() {
    return "PONG\n";
}


// =============================================================
// REPLICATION FORMATTERS
// =============================================================

std::string format_repl_set(
    const std::string& key,
    const std::string& value,
    const std::optional<int> ttl_seconds
) {
    if (ttl_seconds.has_value()) {
        return "REPL SET " +
               key +
               ' ' +
               value +
               " EX " +
               std::to_string(
                   ttl_seconds.value()
               ) +
               '\n';
    }

    return "REPL SET " +
           key +
           ' ' +
           value +
           '\n';
}


std::string format_repl_delete(
    const std::string& key
) {
    return "REPL DELETE " +
           key +
           '\n';
}


std::string format_repl_end() {
    return "REPL END\n";
}

}  // namespace keyvault