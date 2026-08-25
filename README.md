# KeyVault

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

## Interactive commands

After starting KeyVault, enter one command per line:

```text
PUT name Donna Banerjee
GET name
SIZE
DELETE name
EXIT
```

Commands are not case-sensitive, so `get name` and `GET name` behave the same.
Keys cannot contain spaces yet, but values can contain spaces.

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
KeyVault is running. Type HELP to see the commands.
keyvault>
```
