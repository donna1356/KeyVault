# KeyVault
<<<<<<< HEAD

KeyVault is a distributed key-value database written in C++.

The project is being built in small milestones. It currently contains a
single-node, in-memory key-value engine.

## Current operations

- `put(key, value)` stores a new key-value pair or updates an existing key.
- `get(key)` returns the stored value when the key exists.
- `remove(key)` deletes a key-value pair.
- `size()` returns the number of stored keys.

The data currently lives only in memory. It will disappear when the program
stops. Persistence will be added in a later milestone.

## Requirements

- A compiler that supports C++20
- CMake 3.20 or newer

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/keyvault
```

Expected output:

```text
Stored two key-value pairs.
GET name: Donna
DELETE project: success
GET project: key not found
Keys remaining: 1
```
=======
Distributed Key-Value Database
>>>>>>> 60636eeba1ecea8db588f489e3aec1f7bbabbb19
