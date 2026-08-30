#include "keyvault/protocol.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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

bool send_all(int socket_fd, const std::string& message) {
    std::size_t sent = 0;

    while (sent < message.size()) {
        const ssize_t result =
            send(socket_fd, message.data() + sent, message.size() - sent, 0);

        if (result <= 0) {
            return false;
        }

        sent += static_cast<std::size_t>(result);
    }

    return true;
}

std::string read_line(int socket_fd) {
    std::string line;
    char buffer[1];

    while (true) {
        const ssize_t bytes_read = recv(socket_fd, buffer, 1, 0);
        if (bytes_read <= 0) {
            return {};
        }

        if (buffer[0] == '\n') {
            break;
        }

        if (buffer[0] != '\r') {
            line += buffer[0];
        }
    }

    return line;
}

void print_help() {
    std::cout << "Available commands:\n"
              << "  SET <key> <value> [ttl]  Store or update a value\n"
              << "  GET <key>                Read a value\n"
              << "  DELETE <key>             Delete a value\n"
              << "  SIZE                     Show the number of keys\n"
              << "  PING                     Check server connectivity\n"
              << "  HELP                     Show this help message\n"
              << "  EXIT                     Disconnect from the server\n";
}

bool has_extra_argument(std::istringstream& input) {
    std::string extra;
    return static_cast<bool>(input >> extra);
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    std::uint16_t port = 7379;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--host" && index + 1 < argc) {
            host = argv[++index];
        } else if (argument == "--port" && index + 1 < argc) {
            port = static_cast<std::uint16_t>(std::stoi(argv[++index]));
        } else if (argument == "--help") {
            std::cout << "Usage: keyvault-client [--host HOST] [--port PORT]\n";
            return 0;
        }
    }

    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create client socket\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) <= 0) {
        std::cerr << "Invalid host address\n";
        close(socket_fd);
        return 1;
    }

    if (connect(socket_fd,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) < 0) {
        std::cerr << "Failed to connect to " << host << ':' << port << '\n';
        close(socket_fd);
        return 1;
    }

    std::cout << "Connected to KeyVault at " << host << ':' << port
              << ". Type HELP to see the commands.\n";

    std::string line;
    while (true) {
        std::cout << "keyvault> " << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << "\nInput closed. Disconnecting.\n";
            break;
        }

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command.empty()) {
            continue;
        }

        command = to_upper(command);

        if (command == "HELP") {
            if (has_extra_argument(input)) {
                std::cout << "Usage: HELP\n";
                continue;
            }

            print_help();
            continue;
        }

        if (command == "EXIT") {
            if (has_extra_argument(input)) {
                std::cout << "Usage: EXIT\n";
                continue;
            }

            std::cout << "Disconnecting.\n";
            break;
        }

        if (!send_all(socket_fd, line + '\n')) {
            std::cerr << "Failed to send command to server\n";
            break;
        }

        const std::string response = read_line(socket_fd);
        if (response.empty()) {
            std::cerr << "Server closed the connection\n";
            break;
        }

        if (response.rfind("VALUE ", 0) == 0) {
            std::cout << response.substr(6) << '\n';
        } else if (response == "OK") {
            std::cout << "OK\n";
        } else if (response == "NOT_FOUND") {
            std::cout << "Key not found\n";
        } else if (response == "PONG") {
            std::cout << "PONG\n";
        } else if (response.rfind("ERROR ", 0) == 0) {
            std::cout << response.substr(6) << '\n';
        } else {
            std::cout << response << '\n';
        }
    }

    close(socket_fd);
    return 0;
}
