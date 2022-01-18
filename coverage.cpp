#include "entry_point.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::UnorderedElementsAre;

namespace coverage {
    class MethodUsedException : std::exception {
    };

    struct MyNonCopyable {
        MyNonCopyable() {}
        MyNonCopyable(MyNonCopyable&&) {}
        MyNonCopyable(const MyNonCopyable&) { throw MethodUsedException(); }

        bool operator==(const MyNonCopyable &o) const {
            return false;
        }
    };

    TEST(Public, CreateNotThrow) {
        ASSERT_NO_THROW((HashMap<int, int>()));
    }

    TEST(Public, CreateWithValuesNotThrow) {
        ASSERT_NO_THROW((HashMap<int, int>({})));
    }

    TEST(Public, CreateWithAllArgumentsNotThrow) {
        ASSERT_NO_THROW((HashMap<int, int>({}, std::hash<int>(), std::equal_to<int>())));
    }

    TEST(Public, SizeAfterCreationEqualsZero) {
        auto map = HashMap<int, int>();

        ASSERT_EQ(map.size(), 0);
    }

    TEST(Public, MapIsEmptyAfterCreation) {
        auto map = HashMap<int, int>();

        EXPECT_THAT(map, UnorderedElementsAre());
    }

    TEST(Public, MapIsFilledWithPairsGivenToCtor) {
        auto map = HashMap<int, int>({std::make_pair(1, 2),
                                      std::make_pair(3, 4),
                                      std::make_pair(5, 6),
                                      std::make_pair(7, 8)});

        EXPECT_THAT(map, UnorderedElementsAre(std::make_pair(1, 2),
                                              std::make_pair(3, 4),
                                              std::make_pair(5, 6),
                                              std::make_pair(7, 8)));
    }

    TEST(Public, MapIteratorsWorks) {
        auto map = HashMap<int, int>({std::make_pair(1, 2),
                                      std::make_pair(3, 4),
                                      std::make_pair(5, 6),
                                      std::make_pair(7, 8)});

        std::vector<std::pair<int, int>> p;
        for (auto &it : map) {
            p.push_back(it);
        }

        EXPECT_THAT(p, UnorderedElementsAre(
            std::make_pair(1, 2),
            std::make_pair(3, 4),
            std::make_pair(5, 6),
            std::make_pair(7, 8)));
    }

    TEST(Public, MapConstIteratosWorks) {
        auto map = HashMap<int, int>({std::make_pair(1, 2),
                                      std::make_pair(3, 4),
                                      std::make_pair(5, 6),
                                      std::make_pair(7, 8)});

        std::vector<std::pair<int, int>> p;
        for (const auto &it : map) {
            p.push_back(it);
        }

        EXPECT_THAT(p, UnorderedElementsAre(
            std::make_pair(1, 2),
            std::make_pair(3, 4),
            std::make_pair(5, 6),
            std::make_pair(7, 8)));
    }

    TEST(Public, IndexingReturnsValueOfKey) {
        auto map = HashMap<int, int>({std::make_pair<int, int>(1, 4)});

        ASSERT_EQ(map[1], 4);
    }

    TEST(Public, IndexingUnexistingKeyCreatesDefaultValue) {
        auto map = HashMap<int, int>();

        ASSERT_EQ(map[0], 0);
    }

    TEST(Public, MapUsesGivenHasher) {
        struct MyHash {
            size_t operator()(const int &i) const {
                throw MethodUsedException();
            }
        };

        auto map = HashMap<int, int, MyHash>({}, MyHash());

        ASSERT_THROW(map.insert(std::make_pair(1, 1)), MethodUsedException);
    }

    TEST(Public, MapUsesGivenComparer) {
        struct MyComparer {
            bool operator()(const int &i, const int &j) const {
                throw MethodUsedException();
            }
        };

        auto map = HashMap<int, int, std::hash<int>, MyComparer>({}, std::hash<int>(), MyComparer());

        map.insert(std::make_pair(1, 4));

        ASSERT_THROW(map.insert(std::make_pair(1, 2)), MethodUsedException);
    }

    TEST(Public, InsertPairNewElementReturnsRightValue) {
        auto map = HashMap<int, int>();

        auto[inserted, iterator] = map.insert(std::make_pair(1, 4));

        ASSERT_TRUE(inserted);
        ASSERT_EQ(iterator->first, 1);
        ASSERT_EQ(iterator->second, 4);
    }

    TEST(Public, InsertPairExistingKeyDoNothing) {
        auto map = HashMap<int, int>({std::make_pair(1, 2)});

        auto[inserted, iterator] = map.insert(std::make_pair(1, 4));

        ASSERT_FALSE(inserted);
        ASSERT_EQ(map[1], 2);
        ASSERT_EQ(iterator, map.end());
    }

    TEST(Public, InsertingByRvalueOfValueWillNotCreateCopy) {
        auto map = HashMap<int, MyNonCopyable>();

        ASSERT_NO_THROW(map.insert(std::make_pair(1, MyNonCopyable())));
    }

    TEST(Public, InsertingByRValueOfValueWithoutPairWillNotCreateCopy) {
        auto map = HashMap<int, MyNonCopyable>();

        ASSERT_NO_THROW(map.insert(1, MyNonCopyable()));
    }

    TEST(Public, InsertingByRValueKeyWillNotCreateCopy) {
        struct Hasher {
            size_t operator()(const MyNonCopyable &o) const {
                return 0;
            }
        };

        auto map = HashMap<MyNonCopyable, int, Hasher>();

        ASSERT_NO_THROW(map.insert(MyNonCopyable(), 1));
    }

    TEST(Public, InsertingByIndexingWillNotCreateCopy) {
        struct Hasher {
            size_t operator()(const MyNonCopyable &o) const {
                return 0;
            }
        };

        auto map = HashMap<MyNonCopyable, int, Hasher>();

        ASSERT_NO_THROW(map[MyNonCopyable()]);
    }

    TEST(Public, FindWillReturnIteratorToEndIfNotFound) {
        auto map = HashMap<int, int>({
            std::make_pair(1, 2),
            std::make_pair(3, 4)
        });

        auto it = map.find(4);

        ASSERT_TRUE(it == map.end());
    }

    TEST(Public, FindWillReturnIteratorPointingToGivenKey) {
        auto map = HashMap<int, int>({
            std::make_pair(1, 2),
            std::make_pair(3, 4)
        });

        auto it = map.find(1);

        ASSERT_EQ(it->first, 1);
        ASSERT_EQ(it->second, 2);
    }


    TEST(Public, EraseWillDeleteElement) {
        auto map = HashMap<int, int>({
            std::make_pair(1, 2),
            std::make_pair(2, 3),
            std::make_pair(5, 6)
        });

        auto it = map.find(2);

        map.erase(it);

        EXPECT_THAT(map, UnorderedElementsAre(
            std::make_pair(1, 2),
            std::make_pair(5, 6)
        ));
    }

    TEST(Public, EraseCanClearMap) {
        auto map = HashMap<int, int>({
            std::make_pair(1, 2),
            std::make_pair(2, 3),
            std::make_pair(5, 6)
        });

        auto current = map.begin();

        while (map.size() != 0) {
            current = map.erase(current);
        }

        EXPECT_THAT(map, UnorderedElementsAre());
    }

    TEST(Public, InsertByRValueOfKeyAndValueWillNotCreateCopy) {
        struct Hasher {
            size_t operator()(const MyNonCopyable &o) {
                return 0;
            }
        };

        auto map = HashMap<MyNonCopyable, MyNonCopyable, Hasher>();

        ASSERT_NO_THROW(map.insert(MyNonCopyable(), MyNonCopyable()));
    }

    TEST(Public, TryEmplaceNoCallsCopyOrMoveOfValue) {
        struct OnlyConstruct {
            OnlyConstruct(int arg) {}
            OnlyConstruct(const OnlyConstruct &o) { throw MethodUsedException(); }
            OnlyConstruct(OnlyConstruct &&o) { throw MethodUsedException(); }
        };

        auto map = HashMap<int, OnlyConstruct>();

        ASSERT_NO_THROW(map.try_emplace(1, 42));
    }

    TEST(Public, TryEmplaceReturnsFalseIfElementWithKeyAlreadyExists) {
        auto map = HashMap<int, int>();

        map.insert(1, 2);

        auto [created, _] = map.try_emplace(1, 2);

        ASSERT_FALSE(created);
    }

    TEST(Public, TryEmplaceReturnsTruIfElementWithKeyNotExists) {
        auto map = HashMap<int, int>();

        auto [created, _] = map.try_emplace(1, 2);

        ASSERT_TRUE(created);
    }

    TEST(Pulic, TryEmplaceReturnsRightIterator) {
        auto map = HashMap<int, int>();

        auto [_, it] = map.try_emplace(3);

        ASSERT_EQ(it->first, 3);
        ASSERT_EQ(it->second, 0);
    }
}
