#include "keyvault/key_value_store.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string toUpper(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
    }

    return text;
}

void printHelp() {
    std::cout << "Available commands:\n"
              << "  PUT <key> <value>  Store or update a value\n"
              << "  GET <key>          Read a value\n"
              << "  DELETE <key>       Delete a value\n"
              << "  SIZE               Show the number of keys\n"
              << "  HELP               Show this help message\n"
              << "  EXIT               Stop KeyVault\n";
}

bool hasExtraArgument(std::istringstream& input) {
    std::string extra;
    return static_cast<bool>(input >> extra);
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

        command = toUpper(command);

        if (command == "PUT") {
            std::string key;
            std::string value;

            input >> key;
            std::getline(input >> std::ws, value);

            if (key.empty() || value.empty()) {
                std::cout << "Usage: PUT <key> <value>\n";
                continue;
            }

            store.put(key, value);
            std::cout << "OK\n";
        } else if (command == "GET") {
            std::string key;

            if (!(input >> key) || hasExtraArgument(input)) {
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

            if (!(input >> key) || hasExtraArgument(input)) {
                std::cout << "Usage: DELETE <key>\n";
                continue;
            }

            if (store.remove(key)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Key not found\n";
            }
        } else if (command == "SIZE") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: SIZE\n";
                continue;
            }

            std::cout << store.size() << '\n';
        } else if (command == "HELP") {
            if (hasExtraArgument(input)) {
                std::cout << "Usage: HELP\n";
                continue;
            }

            printHelp();
        } else if (command == "EXIT") {
            if (hasExtraArgument(input)) {
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
