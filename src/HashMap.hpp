#include <cstring>
#include <functional>
#include <initializer_list>
#include <cstddef>

#include "PrimesHelper.h"

template<class TKey, class TValue, class Hasher = std::hash<TKey>, class KeyEqualComparer = std::equal_to<TKey>>
class HashMap {
public:
    class Iterator;

    using KeyValuePair = std::pair<const TKey, TValue>;

    using InsertionResult = std::pair<bool, Iterator>;

    class Iterator {
    public:
        Iterator(HashMap *map, int entry) : map(map) {
            currentEntryIndex = entry;

            if (entry != -1) {
                currentBucketIndex = static_cast<int>(map->entries[currentEntryIndex].hash % map->capacity);
            } else {
                currentBucketIndex = -1;
            }
        }

        Iterator(const Iterator &other) = default;

        Iterator(Iterator &&other) noexcept = default;

        KeyValuePair &operator*() const {
            return *map->entries[currentEntryIndex].kvp;
        }

        KeyValuePair *operator->() const {
            return map->entries[currentEntryIndex].kvp;
        }

        Iterator &operator++() {
            if (currentEntryIndex == -1) {
                return *this;
            }

            if (map->entries[currentEntryIndex].next != -1) {
                currentEntryIndex = map->entries[currentEntryIndex].next;
            } else {
                currentBucketIndex++;
                const auto capacity = static_cast<int>(map->capacity);

                while (currentBucketIndex < capacity && map->buckets[currentBucketIndex] == -1) {
                    currentBucketIndex++;
                }

                if (currentBucketIndex < capacity) {
                    currentEntryIndex = map->buckets[currentBucketIndex];
                } else {
                    currentEntryIndex = -1;
                }
            }

            return *this;
        }

        Iterator operator++(int) {
            return Iterator(++this);
        }

        bool operator==(const Iterator &other) const {
            return currentEntryIndex == other.currentEntryIndex;
        }

        bool operator!=(const Iterator &other) const {
            return currentEntryIndex != other.currentEntryIndex;
        }

        Iterator &operator=(const Iterator &other) = default;

        Iterator &operator=(Iterator &&other) noexcept = default;

        [[nodiscard]] int GetEntryIndex() const {
            return currentEntryIndex;
        }

    private:
        HashMap *map;
        int currentBucketIndex;
        int currentEntryIndex;
    };

    class ConstIterator {
    public:
        explicit ConstIterator(Iterator wrapped) : wrapped(wrapped) {
        }

        ConstIterator(const ConstIterator &other) = default;

        ConstIterator(ConstIterator &&other) noexcept = default;

        const KeyValuePair &operator*() const {
            return wrapped.operator*();
        }

        const KeyValuePair *operator->() const {
            return wrapped.operator->();
        }

        ConstIterator &operator++() {
            ++wrapped;

            return *this;
        }

        ConstIterator operator++(int) {
            return ConstIterator(++wrapped);
        }

        bool operator==(const ConstIterator &other) const {
            return wrapped == other.wrapped;
        }

        bool operator!=(const ConstIterator &other) const {
            return wrapped != other.wrapped;
        }

        ConstIterator &operator=(const ConstIterator &other) = default;

        ConstIterator &operator=(ConstIterator &&other) noexcept = default;

    private:
        Iterator wrapped;
    };

    using value_type = KeyValuePair;
    using const_iterator = ConstIterator;

    HashMap() {
        buckets = nullptr;
        entries = nullptr;
        capacity = usedEntriesAmount = deletedEntriesAmount = 0;
        deletedList = -1;
    }

    HashMap(std::initializer_list<KeyValuePair> values,
            const Hasher &hasher = Hasher(),
            const KeyEqualComparer &keyEqualComparer = KeyEqualComparer()) : hasher(hasher), keyEqualComparer(keyEqualComparer) {
        buckets = nullptr;
        entries = nullptr;
        capacity = usedEntriesAmount = deletedEntriesAmount = 0;
        deletedList = -1;

        for (const auto &kvp : values) {
            insert(kvp);
        }
    }

    HashMap(const HashMap &other) {
        buckets = nullptr;
        entries = nullptr;
        capacity = usedEntriesAmount = deletedEntriesAmount = 0;
        deletedList = -1;

        for (const auto &kvp : other) {
            insert(kvp);
        }
    }

    HashMap(HashMap &&other) noexcept {
        buckets = nullptr;
        entries = nullptr;
        capacity = usedEntriesAmount = deletedEntriesAmount = 0;
        deletedList = -1;

        MoveFrom(other);
    }

    ~HashMap() {
        clear();
    }

    [[nodiscard]] size_t size() const {
        return usedEntriesAmount - deletedEntriesAmount;
    }

    void clear() {
        delete[] buckets;
        delete[] entries;
        buckets = nullptr;
        entries = nullptr;
        capacity = usedEntriesAmount = deletedEntriesAmount = 0;
        deletedList = -1;
    }

    InsertionResult insert(const KeyValuePair &item) {
        return insert(KeyValuePair(item));
    }

    InsertionResult insert(TKey &&key, TValue &&value) {
        const auto hash = std::invoke(hasher, key);
        const auto existing = TryFindEntryIndex(key, hash);

        if (existing != -1) {
            return std::make_pair(false, end());
        }

        const auto createdEntryIndex = CreateAndGetEntryIndex(
            hash, std::forward<TKey>(key), std::forward<TValue>(value));

        return std::make_pair(true, Iterator(this, createdEntryIndex));
    }

    InsertionResult insert(KeyValuePair &&item) {
        const auto hash = std::invoke(hasher, item.first);
        const auto existing = TryFindEntryIndex(item.first, hash);

        if (existing != -1) {
            return std::make_pair(false, end());
        }

        const auto createdEntryIndex = CreateAndGetEntryIndex(std::forward<KeyValuePair>(item), hash);

        return std::make_pair(true, Iterator(this, createdEntryIndex));
    }

    template <class...Args>
    InsertionResult try_emplace(const TKey &key, Args&&... args) {
        return try_emplace(TKey(key), std::forward<Args>(args)...);
    }

    template <class...Args>
    InsertionResult try_emplace(TKey&& key, Args&&... args) {
        const auto hash = std::invoke(hasher, key);
        const auto existing = TryFindEntryIndex(key, hash);

        if (existing != -1) {
            return std::make_pair(false, end());
        }

        const auto createdEntryIndex = CreateAndGetEntryIndex(
            hash, std::forward<TKey>(key), std::forward<Args>(args)...);

        return std::make_pair(true, Iterator(this, createdEntryIndex));
    }

    TValue &operator[](const TKey &key) {
        const auto hash = std::invoke(hasher, key);
        const auto existingIndex = TryFindEntryIndex(key, hash);

        if (existingIndex != -1) {
            return entries[existingIndex].kvp->second;
        }

        const auto createdIndex = CreateAndGetEntryIndex(hash, TKey(key), TValue());

        return entries[createdIndex].kvp->second;
    }

    TValue &operator[](TKey &&key) {
        const auto hash = std::invoke(hasher, key);
        const auto existingIndex = TryFindEntryIndex(key, hash);

        if (existingIndex != -1) {
            return entries[existingIndex].kvp->second;
        }

        const auto createdIndex = CreateAndGetEntryIndex(hash, std::forward<TKey>(key), TValue());

        return entries[createdIndex].kvp->second;
    }

    Iterator erase(Iterator position) {
        const auto entryIndex = position.GetEntryIndex();
        auto bucket = static_cast<int>(entries[entryIndex].hash % capacity);
        const auto previousIndex = FindPreviousIndexOf(bucket, entryIndex);

        if (previousIndex != -1) {
            entries[previousIndex].next = entries[entryIndex].next;
        } else {
            buckets[bucket] = entries[entryIndex].next;
        }

        const auto next = entries[entryIndex].next;

        delete entries[entryIndex].kvp;
        entries[entryIndex].kvp = nullptr;
        entries[entryIndex].next = deletedList;
        deletedList = entryIndex;
        deletedEntriesAmount++;

        if (next != -1) {
            return Iterator(this, next);
        }

        while (++bucket < capacity && buckets[bucket] == -1) {
            ;
        }

        if (bucket < capacity) {
            return Iterator(this, buckets[bucket]);
        }

        return end();
    }

    Iterator find(const TKey &key) {
        auto index = TryFindEntryIndex(key, std::invoke(hasher, key));

        return index != -1 ? Iterator(this, index) : end();
    }

    ConstIterator find(const TKey &key) const {
        auto nonConstUnwrapped = const_cast<HashMap *>(this);

        return ConstIterator(nonConstUnwrapped->find(key));
    }

    HashMap &operator=(const HashMap &other) {
        if (&other != this) {
            clear();

            for (const auto &kvp : other) {
                insert(kvp);
            }
        }

        return *this;
    }

    HashMap &operator=(HashMap &&other) noexcept {
        if (&other != this) {
            clear();

            MoveFrom(other);
        }

        return *this;
    }

    Iterator begin() {
        auto firstBucket = 0;

        while (firstBucket < capacity && buckets[firstBucket] == -1) {
            firstBucket++;
        }

        return Iterator(this, firstBucket < capacity ? buckets[firstBucket] : -1);
    }

    Iterator end() {
        return Iterator(this, -1);
    }

    ConstIterator begin() const {
        auto nonConstUnwrapped = const_cast<HashMap *>(this);

        return ConstIterator(nonConstUnwrapped->begin());
    }

    ConstIterator end() const {
        auto nonConstUnwrapped = const_cast<HashMap *>(this);

        return ConstIterator(nonConstUnwrapped->end());
    }

    ConstIterator cbegin() const {
        return begin();
    }

    ConstIterator cend() const {
        return end();
    }

private:
    struct Entry {
        Entry() {
            hash = 0;
            next = -1;
            kvp = nullptr;
        }

        Entry(const Entry &other) = delete;

        Entry(Entry &&other) noexcept {
            hash = other.hash;
            next = other.next;
            kvp = other.kvp;
            other.kvp = nullptr;
        }

        ~Entry() {
            delete kvp;
        }

        Entry &operator=(const Entry &other) = delete;

        Entry &operator=(Entry &&other) noexcept {
            if (&other != this) {
                hash = other.hash;
                next = other.next;

                delete kvp;
                kvp = other.kvp;
                other.kvp = nullptr;
            }

            return *this;
        }

        size_t hash;
        KeyValuePair *kvp;
        int next;
    };

    Hasher hasher;
    KeyEqualComparer keyEqualComparer;
    int *buckets;
    Entry *entries;
    size_t capacity;
    size_t usedEntriesAmount;
    size_t deletedEntriesAmount;
    int deletedList;

    void Initialize(size_t size) {
        capacity = PrimesHelper::GetPrime(size);

        buckets = new int[capacity];
        entries = new Entry[capacity];

        std::memset(buckets, -1, sizeof(int) * capacity);
    }

    void MoveFrom(HashMap &&other) {
        hasher = std::move(other.hasher);
        keyEqualComparer = std::move(other.keyEqualComparer);

        usedEntriesAmount = other.usedEntriesAmount;
        deletedEntriesAmount = other.deletedEntriesAmount;
        deletedList = other.deletedList;

        buckets = other.buckets;
        entries = other.entries;
        capacity = other.capacity;

        other.buckets = other.entries = nullptr;
        other.usedEntriesAmount = other.deletedEntriesAmount = other.capacity = 0;
        other.deletedList = -1;
    }

    int CreateAndGetEntryIndex(KeyValuePair &&pair, size_t hash) {
        auto [bucket, index] = GetNextCreationBucketAndIndex(hash);

        entries[index].hash = hash;
        entries[index].kvp = new KeyValuePair(std::forward<KeyValuePair>(pair));
        entries[index].next = buckets[bucket];

        buckets[bucket] = index;

        return index;
    }

    template <class...Args>
    int CreateAndGetEntryIndex(size_t hash, TKey &&key, Args&&... args) {
        auto [bucket, index] = GetNextCreationBucketAndIndex(hash);

        entries[index].hash = hash;
        entries[index].kvp = new KeyValuePair(
            std::piecewise_construct,
            std::make_tuple(std::forward<TKey>(key)),
            std::make_tuple(std::forward<Args>(args)...));
        entries[index].next = buckets[bucket];

        buckets[bucket] = index;

        return index;
    }

    std::pair<int, int> GetNextCreationBucketAndIndex(size_t hash) {
        if (capacity == 0) {
            Initialize(0);
        }

        int index;
        auto bucket = static_cast<int>(hash % capacity);

        if (deletedEntriesAmount > 0) {
            index = deletedList;
            deletedList = entries[deletedList].next;
            deletedEntriesAmount--;
        } else {
            if (usedEntriesAmount == capacity) {
                Enlarge();
                bucket = static_cast<int>(hash % capacity);
            }

            index = static_cast<int>(usedEntriesAmount++);
        }

        return std::make_pair(bucket, index);
    }

    int TryFindEntryIndex(const TKey &key, size_t hash) {
        if (capacity == 0) {
            return -1;
        }

        auto current = static_cast<int>(buckets[hash % capacity]);

        while (current >= 0) {
            auto &entry = entries[current];

            if (hash == entry.hash && std::invoke(keyEqualComparer, key, entry.kvp->first)) {
                return current;
            }

            current = entry.next;
        }

        return current;
    }

    void Enlarge() {
        const auto oldCapacity = capacity;
        capacity = PrimesHelper::ExpandPrime(capacity);

        if (capacity <= oldCapacity) {
            // unreachable
            return;
        }

        auto *newBuckets = new int[capacity];
        auto *newEntries = new Entry[capacity];

        std::memset(newBuckets, -1, sizeof(int) * capacity);

        for (size_t i = 0; i < oldCapacity; i++) {
            newEntries[i] = std::move(entries[i]);

            if (newEntries[i].kvp != nullptr) {
                const auto newBucket = static_cast<int>(newEntries[i].hash % capacity);
                newEntries[i].next = newBuckets[newBucket];
                newBuckets[newBucket] = static_cast<int>(i);
            }
        }

        delete[] entries;
        delete[] buckets;
        buckets = newBuckets;
        entries = newEntries;
    }

    int FindPreviousIndexOf(int bucket, int entryIndex) {
        if (capacity == 0) {
            return -1;
        }

        const auto &key = entries[entryIndex].kvp->first;
        const auto keyHash = entries[entryIndex].hash;
        auto current = buckets[bucket];
        auto previous = -1;

        while (current < capacity) {
            auto &entry = entries[current];

            if (entry.hash == keyHash && std::invoke(keyEqualComparer, entry.kvp->first, key)) {
                break;
            }

            previous = current;
            current = entries[current].next;
        }

        return previous;
    }
};
