#include "keyvault/key_value_store.hpp"

#include <iostream>

int main() {
    keyvault::KeyValueStore store;

    store.put("name", "Donna");
    store.put("project", "KeyVault");
    std::cout << "Stored two key-value pairs.\n";

    const auto name = store.get("name");
    if (name.has_value()) {
        std::cout << "GET name: " << name.value() << '\n';
    }

    const bool removed = store.remove("project");
    std::cout << "DELETE project: "
              << (removed ? "success" : "key not found") << '\n';

    const auto project = store.get("project");
    if (!project.has_value()) {
        std::cout << "GET project: key not found\n";
    }

    std::cout << "Keys remaining: " << store.size() << '\n';
    return 0;
}
