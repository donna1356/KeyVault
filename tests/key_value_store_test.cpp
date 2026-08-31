#include "keyvault/key_value_store.hpp"
#include "keyvault/lru_cache.hpp"
#include "keyvault/protocol.hpp"
#include "keyvault/replication.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string temp_file_path(const std::string& name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

}  // namespace


// ============================================================
// LRU CACHE TESTS
// ============================================================

TEST(LRUCacheTest, EvictsLeastRecentlyUsedEntry) {
    keyvault::LRUCache<std::string, int> cache(2);

    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);

    EXPECT_FALSE(cache.get("a").has_value());
    EXPECT_EQ(cache.get("b"), 2);
    EXPECT_EQ(cache.get("c"), 3);
}


TEST(LRUCacheTest, GetUpdatesRecency) {
    keyvault::LRUCache<std::string, int> cache(2);

    cache.put("a", 1);
    cache.put("b", 2);

    EXPECT_EQ(cache.get("a"), 1);

    cache.put("c", 3);

    EXPECT_EQ(cache.get("a"), 1);
    EXPECT_FALSE(cache.get("b").has_value());
    EXPECT_EQ(cache.get("c"), 3);
}


// ============================================================
// KEY VALUE STORE TESTS
// ============================================================

TEST(KeyValueStoreTest, PutGetRemove) {
    keyvault::KeyValueStore store;

    store.put("name", "KeyVault");

    EXPECT_EQ(store.get("name"), "KeyVault");
    EXPECT_EQ(store.size(), 1U);

    EXPECT_TRUE(store.remove("name"));

    EXPECT_FALSE(store.get("name").has_value());
    EXPECT_EQ(store.size(), 0U);
}


TEST(KeyValueStoreTest, ExpiresKeysAfterTtl) {
    keyvault::KeyValueStore store;

    store.put("session", "active", 1);

    EXPECT_EQ(store.get("session"), "active");

    std::this_thread::sleep_for(
        std::chrono::seconds(2)
    );

    store.purge_expired();

    EXPECT_FALSE(
        store.get("session").has_value()
    );

    EXPECT_EQ(store.size(), 0U);
}


// Verifies that SIZE does not count logically expired keys
// even before purge_expired() is explicitly called.
TEST(KeyValueStoreTest, SizeDoesNotCountExpiredKeys) {
    keyvault::KeyValueStore store;

    store.put("temporary", "value", 1);

    EXPECT_EQ(store.size(), 1U);

    std::this_thread::sleep_for(
        std::chrono::seconds(2)
    );

    EXPECT_EQ(store.size(), 0U);
}


TEST(KeyValueStoreTest, PersistsDataAcrossLoads) {
    const std::string path =
        temp_file_path(
            "keyvault_test_snapshot.dat"
        );

    std::filesystem::remove(path);

    {
        keyvault::KeyValueStore store;

        store.put(
            "city",
            "Bengaluru"
        );

        ASSERT_TRUE(
            store.save_to_disk(path)
        );
    }

    keyvault::KeyValueStore restored_store;

    ASSERT_TRUE(
        restored_store.load_from_disk(path)
    );

    EXPECT_EQ(
        restored_store.get("city"),
        "Bengaluru"
    );

    std::filesystem::remove(path);
}


// Verify failed loads don't destroy existing in-memory data.
TEST(KeyValueStoreTest, FailedLoadPreservesExistingData) {
    const std::string path =
        temp_file_path(
            "keyvault_file_that_does_not_exist.dat"
        );

    std::filesystem::remove(path);

    keyvault::KeyValueStore store;

    store.put(
        "important",
        "data"
    );

    EXPECT_FALSE(
        store.load_from_disk(path)
    );

    EXPECT_EQ(
        store.get("important"),
        "data"
    );
}


TEST(KeyValueStoreTest, EnforcesLruCapacity) {
    keyvault::KeyValueStore store(2);

    store.put("one", "1");
    store.put("two", "2");

    EXPECT_EQ(
        store.get("one"),
        "1"
    );

    store.put(
        "three",
        "3"
    );

    EXPECT_FALSE(
        store.get("two").has_value()
    );

    EXPECT_EQ(
        store.get("one"),
        "1"
    );

    EXPECT_EQ(
        store.get("three"),
        "3"
    );
}


TEST(KeyValueStoreTest, HandlesConcurrentAccess) {
    keyvault::KeyValueStore store;

    auto writer = [&store]() {
        for (int index = 0;
             index < 100;
             ++index) {

            store.put(
                "counter",
                std::to_string(index)
            );
        }
    };

    auto reader = [&store]() {
        for (int index = 0;
             index < 100;
             ++index) {

            store.get("counter");
            store.size();
        }
    };

    std::vector<std::thread> threads;

    threads.emplace_back(writer);
    threads.emplace_back(reader);
    threads.emplace_back(reader);

    for (std::thread& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(
        store.get("counter").has_value()
    );
}


// ============================================================
// PROTOCOL TESTS
// ============================================================

// Numeric values should remain values.
// "21" must NOT automatically become a TTL.
TEST(ProtocolTest, NumericValueIsNotTtl) {
    const auto command =
        keyvault::parse_command(
            "SET age 21"
        );

    EXPECT_EQ(
        command.type,
        keyvault::Command::Type::Set
    );

    EXPECT_EQ(
        command.key,
        "age"
    );

    EXPECT_EQ(
        command.value,
        "21"
    );

    EXPECT_FALSE(
        command.ttl_seconds.has_value()
    );
}


// Explicit EX syntax should create a TTL.
TEST(ProtocolTest, ParsesExplicitTtl) {
    const auto command =
        keyvault::parse_command(
            "SET session active EX 60"
        );

    EXPECT_EQ(
        command.type,
        keyvault::Command::Type::Set
    );

    EXPECT_EQ(
        command.key,
        "session"
    );

    EXPECT_EQ(
        command.value,
        "active"
    );

    ASSERT_TRUE(
        command.ttl_seconds.has_value()
    );

    EXPECT_EQ(
        command.ttl_seconds.value(),
        60
    );
}


// Invalid TTL should be rejected safely.
TEST(ProtocolTest, RejectsInvalidTtl) {
    const auto command =
        keyvault::parse_command(
            "SET session active EX abc"
        );

    EXPECT_EQ(
        command.type,
        keyvault::Command::Type::Unknown
    );

    EXPECT_FALSE(
        command.error.empty()
    );
}


// Zero TTL should also be rejected.
TEST(ProtocolTest, RejectsZeroTtl) {
    const auto command =
        keyvault::parse_command(
            "SET session active EX 0"
        );

    EXPECT_EQ(
        command.type,
        keyvault::Command::Type::Unknown
    );
}


// ============================================================
// REPLICATION TESTS
// ============================================================

TEST(ReplicationTest, ParsesPeerAddresses) {
    const auto peers =
        keyvault::parse_peer_list(
            "127.0.0.1:7379,"
            "127.0.0.1:7380"
        );

    ASSERT_EQ(
        peers.size(),
        2U
    );

    EXPECT_EQ(
        peers[0].host,
        "127.0.0.1"
    );

    EXPECT_EQ(
        peers[0].port,
        7379
    );

    EXPECT_EQ(
        peers[1].port,
        7380
    );
}


TEST(ReplicationTest, RejectsInvalidPeerAddresses) {
    auto invalid1 =
        keyvault::parse_peer_address(
            "127.0.0.1:abc"
        );

    EXPECT_TRUE(
        invalid1.host.empty()
    );

    EXPECT_EQ(
        invalid1.port,
        0
    );


    auto invalid2 =
        keyvault::parse_peer_address(
            "127.0.0.1:99999"
        );

    EXPECT_TRUE(
        invalid2.host.empty()
    );

    EXPECT_EQ(
        invalid2.port,
        0
    );


    auto invalid3 =
        keyvault::parse_peer_address(
            ":7379"
        );

    EXPECT_TRUE(
        invalid3.host.empty()
    );

    EXPECT_EQ(
        invalid3.port,
        0
    );


    auto invalid4 =
        keyvault::parse_peer_address(
            "127.0.0.1:"
        );

    EXPECT_TRUE(
        invalid4.host.empty()
    );

    EXPECT_EQ(
        invalid4.port,
        0
    );
}


TEST(ReplicationTest, BuildsReplicationCommands) {

    // Permanent value
    EXPECT_EQ(
        keyvault::format_repl_set(
            "name",
            "Donna",
            std::nullopt
        ),
        "REPL SET name Donna\n"
    );


    // Value with TTL
    EXPECT_EQ(
        keyvault::format_repl_set(
            "session",
            "active",
            60
        ),
        "REPL SET session active EX 60\n"
    );


    EXPECT_EQ(
        keyvault::format_repl_delete(
            "name"
        ),
        "REPL DELETE name\n"
    );


    EXPECT_EQ(
        keyvault::format_repl_end(),
        "REPL END\n"
    );
}


// Ensure replicated TTL commands can also be parsed.
TEST(ReplicationTest, ParsesReplicationTtl) {
    const auto command =
        keyvault::parse_command(
            "REPL SET session active EX 60"
        );

    EXPECT_EQ(
        command.type,
        keyvault::Command::Type::ReplSet
    );

    EXPECT_EQ(
        command.key,
        "session"
    );

    EXPECT_EQ(
        command.value,
        "active"
    );

    ASSERT_TRUE(
        command.ttl_seconds.has_value()
    );

    EXPECT_EQ(
        command.ttl_seconds.value(),
        60
    );
}