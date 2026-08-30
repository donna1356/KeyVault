# KeyVault

KeyVault is a distributed in-memory key-value store written in C++. It uses a
client-server architecture over TCP sockets, with TTL-based expiration, LRU
caching, persistent storage, and thread-safe concurrent request handling.

## Features

- **Distributed multi-node cluster** with peer replication
- **Client-server architecture** over TCP sockets
- **GET, SET, DELETE** operations
- **TTL-based expiration** with background cleanup
- **Persistent storage** via snapshot files
- **LRU caching** with configurable capacity
- **Thread-safe concurrent access** using `std::shared_mutex`
- **Google Test** unit tests

## Requirements

- A compiler that supports C++20
- CMake 3.20 or newer
- Internet access on first build (Google Test is fetched automatically)

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Run a distributed cluster

Start three nodes that replicate writes to each other:

```bash
# Terminal 1 - primary node
./build/keyvault-server --port 7379 --data-file node1.dat \
  --peers 127.0.0.1:7380,127.0.0.1:7381

# Terminal 2
./build/keyvault-server --port 7380 --data-file node2.dat \
  --peers 127.0.0.1:7379,127.0.0.1:7381

# Terminal 3 - joins cluster and syncs existing data from node 1
./build/keyvault-server --port 7381 --data-file node3.dat \
  --peers 127.0.0.1:7379,127.0.0.1:7380 \
  --seed-peer 127.0.0.1:7379
```

How distribution works:

- Multiple independent server **nodes** form a cluster
- `SET` and `DELETE` are **replicated** to all configured peers
- A new node can **sync** existing data from a peer on startup via `--seed-peer`
- Clients can connect to **any node** in the cluster

This is a basic active-replication model (not full consensus), but it is a genuine
multi-node distributed setup.

## Run a single server

```bash
./build/keyvault-server --port 7379 --data-file keyvault.dat --capacity 1000
```

The server:

- listens for TCP connections on the configured port
- loads existing data from disk on startup
- saves data to disk on shutdown
- runs a background thread to purge expired keys

## Run the client

In another terminal:

```bash
./build/keyvault-client --host 127.0.0.1 --port 7379
```

### Client commands

```text
SET name Donna Banerjee
SET session active 30
GET name
SIZE
DELETE name
PING
EXIT
```

`SET` accepts an optional TTL in seconds. Keys expire automatically after the
TTL elapses.

## Local mode

For quick local testing without networking:

```bash
./build/keyvault
```

The local CLI supports `SET`, `GET`, `DELETE`, `SIZE`, `SAVE`, and `LOAD`.

## Project layout

```text
include/keyvault/
  key_value_store.hpp   Core store with TTL, LRU, and persistence
  lru_cache.hpp         LRU cache implementation
  persistence.hpp       Snapshot file I/O
  protocol.hpp          Text protocol parsing and formatting
  replication.hpp     Multi-node peer replication
  server.hpp            Multithreaded TCP server

src/
  key_value_store.cpp
  persistence.cpp
  protocol.cpp
  replication.cpp
  server.cpp
  main.cpp              Local CLI
  server_main.cpp       Server entry point
  client_main.cpp       TCP client CLI

tests/
  key_value_store_test.cpp
```

## Protocol

The wire protocol is line-based:

| Command | Example | Response |
|---|---|---|
| SET | `SET key value` | `OK` |
| SET with TTL | `SET key value 30` | `OK` |
| GET | `GET key` | `VALUE <value>` or `NOT_FOUND` |
| DELETE | `DELETE key` | `OK` or `NOT_FOUND` |
| SIZE | `SIZE` | `<count>` |
| PING | `PING` | `PONG` |
