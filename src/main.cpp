#include "keyvault/key_value_store.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

namespace {

std::string toUpper(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            toupper(static_cast<unsigned char>(character)));
    }

    return text;
}

void printHelp() {
    cout << "Available commands:\n"
              << "  PUT <key> <value>  Store or update a value\n"
              << "  GET <key>          Read a value\n"
              << "  DELETE <key>       Delete a value\n"
              << "  SIZE               Show the number of keys\n"
              << "  HELP               Show this help message\n"
              << "  EXIT               Stop KeyVault\n";
}

bool hasExtraArgument(std::istringstream& input) {
    string extra;
    return static_cast<bool>(input >> extra);
}

}  // namespace

int main() {
    keyvault::KeyValueStore store;

    cout << "KeyVault is running. Type HELP to see the commands.\n";

    string line;

    while (true) {
        std::cout << "keyvault> " << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput closed. Stopping KeyVault.\n";
            break;
        }

        istringstream input(line);
        string command;
        input >> command;

        if (command.empty()) {
            continue;
        }

        command = toUpper(command);

        if (command == "PUT") {
            string key;
            string value;

            input >> key;
            getline(input >> ws, value);

            if (key.empty() || value.empty()) {
                std::cout << "Usage: PUT <key> <value>\n";
                continue;
            }

            store.put(key, value);
            cout << "OK\n";
        } else if (command == "GET") {
            string key;

            if (!(input >> key) || hasExtraArgument(input)) {
                std::cout << "Usage: GET <key>\n";
                continue;
            }

            const auto value = store.get(key);
            if (value.has_value()) {
                cout << value.value() << '\n';
            } else {
                cout << "Key not found\n";
            }
        } else if (command == "DELETE") {
            string key;

            if (!(input >> key) || hasExtraArgument(input)) {
                cout << "Usage: DELETE <key>\n";
                continue;
            }

            if (store.remove(key)) {
                cout << "OK\n";
            } else {
                std::cout << "Key not found\n";
            }
        } else if (command == "SIZE") {
            if (hasExtraArgument(input)) {
                cout << "Usage: SIZE\n";
                continue;
            }

            cout << store.size() << '\n';
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

            cout << "Stopping KeyVault.\n";
            break;
        } else {
            cout << "Unknown command. Type HELP to see the commands.\n";
        }
    }

    return 0;
}
