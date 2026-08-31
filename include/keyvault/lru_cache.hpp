#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace keyvault {

template <typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(std::size_t capacity)
        : capacity_(capacity) {}

    void put(const Key& key, Value value) {
        if (capacity_ == 0) {
            return;
        }

        auto found = map_.find(key);

        if (found != map_.end()) {
            found->second->second = std::move(value);
            items_.splice(items_.begin(), items_, found->second);
            return;
        }

        items_.emplace_front(key, std::move(value));
        map_[key] = items_.begin();

        if (items_.size() > capacity_) {
            auto last = std::prev(items_.end());
            map_.erase(last->first);
            items_.pop_back();
        }
    }

    std::optional<Value> get(const Key& key) {
        auto found = map_.find(key);

        if (found == map_.end()) {
            return std::nullopt;
        }

        items_.splice(items_.begin(), items_, found->second);

        return found->second->second;
    }

    bool remove(const Key& key) {
        auto found = map_.find(key);

        if (found == map_.end()) {
            return false;
        }

        items_.erase(found->second);
        map_.erase(found);

        return true;
    }

    std::size_t size() const {
        return items_.size();
    }

    void clear() {
        items_.clear();
        map_.clear();
    }

    std::vector<std::pair<Key, Value>> entries() const {
        std::vector<std::pair<Key, Value>> result;
        result.reserve(items_.size());

        for (const auto& item : items_) {
            result.push_back(item);
        }

        return result;
    }

private:
    using List = std::list<std::pair<Key, Value>>;
    using Iterator = typename List::iterator;

    std::size_t capacity_;
    List items_;
    std::unordered_map<Key, Iterator> map_;
};

} // namespace keyvault