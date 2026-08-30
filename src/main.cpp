#include "keyvault/key_value_store.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string to_upper(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
    }

    return text;
}

void print_help() {
    std::cout << "Available commands:\n"
              << "  SET <key> <value> [ttl]  Store or update a value\n"
              << "  GET <key>                Read a value\n"
              << "  DELETE <key>             Delete a value\n"
              << "  SIZE                     Show the number of keys\n"
              << "  SAVE <path>              Persist data to disk\n"
              << "  LOAD <path>              Load data from disk\n"
              << "  HELP                     Show this help message\n"
              << "  EXIT                     Stop KeyVault\n";
}

bool has_extra_argument(std::istringstream& input) {
    std::string extra;
    return static_cast<bool>(input >> extra);
}

std::optional<int> parse_ttl(std::string& value) {
    const std::size_t last_space = value.find_last_of(' ');
    if (last_space == std::string::npos) {
        return std::nullopt;
    }

    const std::string ttl_token = value.substr(last_space + 1);
    if (ttl_token.empty()) {
        return std::nullopt;
    }

    for (const char character : ttl_token) {
        if (!std::isdigit(static_cast<unsigned char>(character))) {
            return std::nullopt;
        }
    }

    const int ttl_seconds = std::stoi(ttl_token);
    value = value.substr(0, last_space);
    if (value.empty()) {
        return std::nullopt;
    }

    return ttl_seconds;
}

}  // namespace

int main() {
    keyvault::KeyValueStore store;

    std::cout << "KeyVault is running. Type HELP to see the commands.\n";

    std::string line;

    while (true) {
        std::cout << "keyvault> " << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput closed. Stopping KeyVault.\n";
            break;
        }

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command.empty()) {
            continue;
        }

        command = to_upper(command);

        if (command == "SET" || command == "PUT") {
            std::string key;
            std::string value;

            input >> key;
            std::getline(input >> std::ws, value);

            if (key.empty() || value.empty()) {
                std::cout << "Usage: SET <key> <value> [ttl_seconds]\n";
                continue;
            }

            const auto ttl_seconds = parse_ttl(value);
            if (!ttl_seconds.has_value() && value.empty()) {
                std::cout << "Usage: SET <key> <value> [ttl_seconds]\n";
                continue;
            }

            store.put(key, value, ttl_seconds);
            std::cout << "OK\n";
        } else if (command == "GET") {
            std::string key;

            if (!(input >> key) || has_extra_argument(input)) {
                std::cout << "Usage: GET <key>\n";
                continue;
            }

            const auto value = store.get(key);
            if (value.has_value()) {
                std::cout << value.value() << '\n';
            } else {
                std::cout << "Key not found\n";
            }
        } else if (command == "DELETE") {
            std::string key;

            if (!(input >> key) || has_extra_argument(input)) {
                std::cout << "Usage: DELETE <key>\n";
                continue;
            }

            if (store.remove(key)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Key not found\n";
            }
        } else if (command == "SIZE") {
            if (has_extra_argument(input)) {
                std::cout << "Usage: SIZE\n";
                continue;
            }

            std::cout << store.size() << '\n';
        } else if (command == "SAVE") {
            std::string path;

            if (!(input >> path) || has_extra_argument(input)) {
                std::cout << "Usage: SAVE <path>\n";
                continue;
            }

            if (store.save_to_disk(path)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Failed to save data\n";
            }
        } else if (command == "LOAD") {
            std::string path;

            if (!(input >> path) || has_extra_argument(input)) {
                std::cout << "Usage: LOAD <path>\n";
                continue;
            }

            if (store.load_from_disk(path)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Failed to load data\n";
            }
        } else if (command == "HELP") {
            if (has_extra_argument(input)) {
                std::cout << "Usage: HELP\n";
                continue;
            }

            print_help();
        } else if (command == "EXIT") {
            if (has_extra_argument(input)) {
                std::cout << "Usage: EXIT\n";
                continue;
            }

            std::cout << "Stopping KeyVault.\n";
            break;
        } else {
            std::cout << "Unknown command. Type HELP to see the commands.\n";
        }
    }

    return 0;
}
