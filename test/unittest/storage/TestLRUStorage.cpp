#include "../../src/storage/LRUStorage.h"
#include <bcos-framework/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

namespace std
{
inline ostream& operator<<(ostream& os, const std::optional<bcos::storage::Entry>& entry)
{
    os << entry.has_value();
    return os;
}

inline ostream& operator<<(ostream& os, const std::optional<bcos::storage::Table>& table)
{
    os << table.has_value();
    return os;
}

inline ostream& operator<<(ostream& os, const std::unique_ptr<bcos::Error>& error)
{
    os << error->what();
    return os;
}

inline ostream& operator<<(ostream& os, const std::tuple<std::string, bcos::crypto::HashType>& pair)
{
    os << std::get<0>(pair) << " " << std::get<1>(pair).hex();
    return os;
}
}  // namespace std

/*
namespace bcos::test
{
using namespace bcos::storage;

class StorageFixture
{
public:
    StorageFixture()
    {
        memoryStorage = std::make_shared<storage::StateStorage>(nullptr);
        BOOST_TEST(memoryStorage != nullptr);
        tableFactory = std::make_shared<executor::LRUStorage>(memoryStorage);
        tableFactory->start();

        BOOST_TEST(tableFactory != nullptr);
    }

    ~StorageFixture() { tableFactory->stop(); }

    std::optional<Table> createDefaultTable()
    {
        std::promise<std::optional<Table>> createPromise;
        tableFactory->asyncCreateTable(
            testTableName, valueField, [&](auto&& error, std::optional<Table> table) {
                BOOST_CHECK(!error);
                createPromise.set_value(table);
            });
        return createPromise.get_future().get();
    }

    std::shared_ptr<crypto::Hash> hashImpl;
    std::shared_ptr<storage::StateStorage> memoryStorage;
    protocol::BlockNumber m_blockNumber = 0;
    std::shared_ptr<executor::LRUStorage> tableFactory;
    std::string testTableName = "t_test";
    std::string keyField = "key";
    std::string valueField = "value";
};

BOOST_FIXTURE_TEST_SUITE(TestStorage, StorageFixture)

BOOST_AUTO_TEST_CASE(create_Table)
{
    std::string tableName("t_test1");
    auto table = tableFactory->openTable(tableName);

    BOOST_TEST(!table);
    auto ret = tableFactory->createTable(tableName, valueField);
    BOOST_TEST(ret);

    table = tableFactory->openTable(tableName);
    BOOST_TEST(table);

    BOOST_CHECK_THROW(tableFactory->createTable(tableName, valueField), bcos::Error);
}

BOOST_AUTO_TEST_CASE(rollback)
{
    auto ret = createDefaultTable();
    BOOST_TEST(ret);
    auto table = tableFactory->openTable(testTableName);

    auto deleteEntry = table->newEntry();
    deleteEntry.setStatus(Entry::DELETED);
    BOOST_CHECK_NO_THROW(table->setRow("name", deleteEntry));

    auto entry = std::make_optional(table->newEntry());
    BOOST_CHECK_NO_THROW(entry->setField("value", "Lili"));
    BOOST_CHECK_NO_THROW(table->setRow("name", *entry));
    entry = table->getRow("name");
    BOOST_TEST(entry);
    BOOST_TEST(entry->dirty() == true);
    BOOST_TEST(entry->getField("value") == "Lili");

    auto savePoint = tableFactory->newRecoder();
    tableFactory->setRecoder(savePoint);

    entry = table->newEntry();
    entry->setField("value", "12345");
    table->setRow("id", *entry);
    entry = table->getRow("id");
    BOOST_TEST(entry);
    entry = table->getRow("name");
    BOOST_TEST(entry);

    auto savePoint1 = tableFactory->newRecoder();
    tableFactory->setRecoder(savePoint1);

    entry = table->newEntry();
    entry->setField("value", "500");
    table->setRow("balance", *entry);

    entry = table->getRow("balance");
    BOOST_TEST(entry);

    entry = table->getRow("name");
    BOOST_TEST(entry);

    auto savePoint2 = tableFactory->newRecoder();
    tableFactory->setRecoder(savePoint2);

    auto deleteEntry2 = std::make_optional(table->newDeletedEntry());
    table->setRow("name", *deleteEntry2);

    entry = table->getRow("name");
    BOOST_CHECK(!entry);

    std::cout << "Try remove balance" << std::endl;
    tableFactory->rollback(savePoint2);
    entry = table->getRow("name");
    BOOST_CHECK_NE(entry->status(), Entry::DELETED);

    tableFactory->rollback(savePoint1);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    entry = table->getRow("balance");
    BOOST_TEST(!entry);

    tableFactory->rollback(savePoint);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    entry = table->getRow("balance");
    BOOST_TEST(!entry);
    entry = table->getRow("id");
    BOOST_TEST(!entry);

    // insert without version
    entry = table->newEntry();
    entry->setField("value", "new record");
    BOOST_CHECK_NO_THROW(table->setRow("id", *entry));
}

BOOST_AUTO_TEST_CASE(rollback2)
{
    auto hash0 = tableFactory->hash(hashImpl);
    // auto savePoint0 = tableFactory->savepoint();
    auto savePoint0 = tableFactory->newRecoder();
    tableFactory->setRecoder(savePoint0);

    auto ret = createDefaultTable();
    BOOST_TEST(ret);
    auto table = tableFactory->openTable(testTableName);

    auto deleteEntry = table->newDeletedEntry();
    table->setRow("name", deleteEntry);
    auto entry = std::make_optional(table->newEntry());
    // entry->setField("key", "name");
    entry->setField("value", "Lili");
    table->setRow("name", *entry);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    // BOOST_TEST(table->dirty() == true);
    BOOST_TEST(entry->dirty() == true);
    BOOST_TEST(entry->getField("value") == "Lili");

    // auto savePoint = tableFactory->savepoint();
    auto savePoint = tableFactory->newRecoder();
    tableFactory->setRecoder(savePoint);

    BOOST_CHECK_GT(tableFactory->capacity(), 0);

    entry = table->newEntry();
    // entry->setField("key", "id");
    entry->setField("value", "12345");
    table->setRow("id", *entry);
    entry = table->getRow("id");
    BOOST_TEST(entry);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    // BOOST_TEST(table->dirty() == true);

    tableFactory->rollback(savePoint);

    entry = table->getRow("name");
    BOOST_TEST(entry);
    entry = table->getRow("balance");
    BOOST_TEST(!entry);
    entry = table->getRow("id");
    BOOST_TEST(!entry);
    // BOOST_TEST(table->dirty() == true);
    tableFactory->rollback(savePoint0);

    entry = table->getRow("name");
    BOOST_CHECK(!entry);

    auto hash00 = tableFactory->hash(hashImpl);
    BOOST_CHECK_EQUAL_COLLECTIONS(hash0.begin(), hash0.end(), hash00.begin(), hash00.end());
    BOOST_TEST(hash00 == hash0);
    table = tableFactory->openTable(testTableName);
    BOOST_TEST(!table);
}

BOOST_AUTO_TEST_CASE(rollback3)
{
    // Test rollback multi state storage
}

BOOST_AUTO_TEST_CASE(hash)
{
    auto ret = createDefaultTable();
    BOOST_TEST(ret);

    tableFactory->setEnableTraverse(true);

    auto table = tableFactory->openTable(testTableName);
    auto entry = std::make_optional(table->newEntry());
    // entry->setField("key", "name");
    entry->setField("value", "Lili");
    BOOST_CHECK_NO_THROW(table->setRow("name", *entry));
    entry = table->getRow("name");
    BOOST_TEST(entry);
    auto tableFactory0 = std::make_shared<StateStorage>(tableFactory);

    entry = std::make_optional(table->newEntry());
    // entry->setField("key", "id");
    entry->setField("value", "12345");
    BOOST_CHECK_NO_THROW(table->setRow("id", *entry));
    entry = table->getRow("id");
    BOOST_TEST(entry);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    // BOOST_TEST(table->dirty() == true);
    auto keys = table->getPrimaryKeys({});
    BOOST_TEST(keys.size() == 2);

    auto entries = table->getRows(keys);
    BOOST_TEST(entries.size() == 2);

    auto dbHash1 = tableFactory->hash(hashImpl);

    auto savePoint = tableFactory->newRecoder();
    tableFactory->setRecoder(savePoint);
    auto idEntry = table->getRow("id");

    auto deletedEntry = std::make_optional(table->newDeletedEntry());
    BOOST_CHECK_NO_THROW(table->setRow("id", *deletedEntry));
    entry = table->getRow("id");
    BOOST_CHECK(!entry);

    tableFactory->rollback(savePoint);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    entry = table->getRow("balance");
    BOOST_TEST(!entry);
    // BOOST_TEST(table->dirty() == true);

    auto dbHash2 = tableFactory->hash(hashImpl);
    BOOST_CHECK_EQUAL(dbHash1.hex(), dbHash2.hex());

    // getPrimaryKeys and getRows
    entry = table->newEntry();
    // entry->setField("key", "id");
    entry->setField("value", "12345");
    BOOST_CHECK_NO_THROW(table->setRow("id", *entry));
    entry = table->getRow("name");
    entry->setField("value", "Wang");
    BOOST_CHECK_NO_THROW(table->setRow("name", *entry));
    entry = table->newEntry();
    // entry->setField("key", "balance");
    entry->setField("value", "12345");
    BOOST_CHECK_NO_THROW(table->setRow("balance", *entry));
    BOOST_TEST(entry);
    keys = table->getPrimaryKeys({});
    BOOST_TEST(keys.size() == 3);

    entries = table->getRows(keys);
    BOOST_TEST(entries.size() == 3);
    entry = table->getRow("name");
    BOOST_TEST(entry);
    entry = table->getRow("balance");
    BOOST_TEST(entry);
    entry = table->getRow("balance1");
    BOOST_TEST(!entry);

    auto nameEntry = table->getRow("name");
    auto deletedEntry2 = std::make_optional(table->newDeletedEntry());
    BOOST_CHECK_NO_THROW(table->setRow("name", *deletedEntry2));
    entry = table->getRow("name");
    BOOST_CHECK(!entry);
    // BOOST_CHECK_EQUAL(entry->status(), Entry::DELETED);
    keys = table->getPrimaryKeys({});
    BOOST_TEST(keys.size() == 2);

    entries = table->getRows(keys);
    BOOST_TEST(entries.size() == 2);

    auto idEntry2 = table->getRow("id");
    auto deletedEntry3 = std::make_optional(table->newDeletedEntry());
    BOOST_CHECK_NO_THROW(table->setRow("id", *deletedEntry3));
    entry = table->getRow("id");
    BOOST_CHECK(!entry);
    // BOOST_CHECK_EQUAL(entry->status(), Entry::DELETED);
    keys = table->getPrimaryKeys({});
    BOOST_TEST(keys.size() == 1);

    entries = table->getRows(keys);
    BOOST_TEST(entries.size() == 1);
    // tableFactory->asyncCommit([](Error::Ptr, size_t) {});
}

BOOST_AUTO_TEST_CASE(open_sysTables)
{
    auto table = tableFactory->openTable(StorageInterface::SYS_TABLES);
    BOOST_TEST(table);
}

BOOST_AUTO_TEST_CASE(openAndCommit)
{
    // auto hashImpl2 = make_shared<Header256Hash>();
    auto memoryStorage2 = std::make_shared<StateStorage>(nullptr);
    auto tableFactory2 = std::make_shared<StateStorage>(memoryStorage2);

    for (int i = 10; i < 20; ++i)
    {
        BOOST_TEST(tableFactory2 != nullptr);

        std::string tableName = "testTable" + boost::lexical_cast<std::string>(i);
        auto key = "testKey" + boost::lexical_cast<std::string>(i);
        tableFactory2->createTable(tableName, "value");
        auto table = tableFactory2->openTable(tableName);

        auto entry = std::make_optional(table->newEntry());
        entry->setField("value", "hello world!");
        table->setRow(key, *entry);

        std::promise<bool> getRow;
        table->asyncGetRow(key, [&](auto&& error, auto&& result) {
            BOOST_CHECK(!error);
            BOOST_CHECK_EQUAL(result->getField("value"), "hello world!");

            getRow.set_value(true);
        });

        getRow.get_future().get();
    }
}

BOOST_AUTO_TEST_CASE(chainLink)
{
    std::vector<StateStorage::Ptr> storages;
    auto valueFields = "value1,value2,value3";

    StateStorage::Ptr prev = nullptr;
    for (int i = 0; i < 20; ++i)
    {
        auto tableStorage = std::make_shared<StateStorage>(prev);
        for (int j = 0; j < 10; ++j)
        {
            auto tableName = "table_" + boost::lexical_cast<std::string>(i) + "_" +
                             boost::lexical_cast<std::string>(j);
            BOOST_CHECK(tableStorage->createTable(tableName, valueFields));

            auto table = tableStorage->openTable(tableName);
            BOOST_TEST(table);

            for (int k = 0; k < 100; ++k)
            {
                auto entry = std::make_optional(table->newEntry());
                auto key =
                    boost::lexical_cast<std::string>(i) + boost::lexical_cast<std::string>(k);
                entry->setField("value1", boost::lexical_cast<std::string>(i));
                entry->setField("value2", boost::lexical_cast<std::string>(j));
                entry->setField("value3", boost::lexical_cast<std::string>(k));
                BOOST_CHECK_NO_THROW(table->setRow(key, *entry));
            }
        }

        prev = tableStorage;
        storages.push_back(tableStorage);
    }

    for (int index = 0; index < 20; ++index)
    {
        auto storage = storages[index];
        // Data count must be 10 * 100 + 10
        tbb::atomic<size_t> totalCount = 0;
        storage->parallelTraverse(false, [&](auto&&, auto&&, auto&&) {
            ++totalCount;
            return true;
        });

        BOOST_CHECK_EQUAL(totalCount, 10 * 100 + 10);  // extra 100 for s_tables

        // Dirty data count must be 10 * 100 + 10
        tbb::atomic<size_t> dirtyCount = 0;
        storage->parallelTraverse(true, [&](auto&&, auto&&, auto&&) {
            ++dirtyCount;
            return true;
        });

        BOOST_CHECK_EQUAL(dirtyCount, 10 * 100 + 10);  // extra 100 for s_tables

        // Low level can't touch high level's data
        for (int i = 0; i < 20; ++i)
        {
            for (int j = 0; j < 10; ++j)
            {
                auto tableName = "table_" + boost::lexical_cast<std::string>(i) + "_" +
                                 boost::lexical_cast<std::string>(j);

                auto table = storage->openTable(tableName);
                if (i > index)
                {
                    BOOST_TEST(!table);
                }
                else
                {
                    BOOST_TEST(table);

                    for (int k = 0; k < 100; ++k)
                    {
                        auto key = boost::lexical_cast<std::string>(i) +
                                   boost::lexical_cast<std::string>(k);

                        auto entry = table->getRow(key);
                        if (i > index)
                        {
                            BOOST_TEST(!entry);
                        }
                        else
                        {
                            BOOST_TEST(entry);

                            BOOST_CHECK_GT(entry->capacityOfHashField(), 0);
                            if (i == index)
                            {
                                BOOST_CHECK_EQUAL(entry->dirty(), true);
                            }
                            else
                            {
                                BOOST_CHECK_EQUAL(entry->dirty(), false);
                            }
                            BOOST_CHECK_EQUAL(
                                entry->getField("value1"), boost::lexical_cast<std::string>(i));
                            BOOST_CHECK_EQUAL(
                                entry->getField("value2"), boost::lexical_cast<std::string>(j));
                            BOOST_CHECK_EQUAL(
                                entry->getField("value3"), boost::lexical_cast<std::string>(k));
                        }
                    }
                }
            }
        }

        // After reading, current storage should include previous storage's data, previous data's
        // dirty should be false
        totalCount = 0;
        tbb::concurrent_vector<std::function<void()>> checks;
        storage->parallelTraverse(false, [&](auto&& table, auto&&, auto&& entry) {
            checks.push_back([index, table, entry] {
                // BOOST_CHECK_NE(tableInfo, nullptr);
                if (table != "s_tables")
                {
                    auto i = boost::lexical_cast<int>(entry.getField("value1"));
                    auto j = boost::lexical_cast<int>(entry.getField("value2"));
                    auto k = boost::lexical_cast<int>(entry.getField("value3"));

                    BOOST_CHECK_LE(i, index);
                    BOOST_CHECK_LE(j, 10);
                    BOOST_CHECK_LE(k, 100);
                }
            });

            ++totalCount;
            return true;
        });

        for (auto& it : checks)
        {
            it();
        }

        BOOST_CHECK_EQUAL(totalCount, (10 * 100 + 10) * (index + 1));

        checks.clear();
        dirtyCount = 0;
        storage->parallelTraverse(true, [&](auto&& table, auto&&, auto&& entry) {
            checks.push_back([index, table, entry]() {
                // BOOST_CHECK_NE(tableInfo, nullptr);
                if (table != "s_tables")
                {
                    auto i = boost::lexical_cast<int>(entry.getField("value1"));
                    auto j = boost::lexical_cast<int>(entry.getField("value2"));
                    auto k = boost::lexical_cast<int>(entry.getField("value3"));

                    if (i == index)
                    {
                        BOOST_CHECK_EQUAL(entry.dirty(), true);
                    }
                    else
                    {
                        BOOST_CHECK_EQUAL(entry.dirty(), false);
                    }

                    BOOST_CHECK_LE(j, 10);
                    BOOST_CHECK_LE(k, 100);
                }
            });

            ++dirtyCount;
            return true;
        });

        for (auto& it : checks)
        {
            it();
        }

        BOOST_CHECK_EQUAL(dirtyCount, 10 * 100 + 10);
    }
}

BOOST_AUTO_TEST_CASE(getRows)
{
    std::vector<StateStorage::Ptr> storages;
    auto valueFields = "value1,value2,value3";

    StateStorage::Ptr prev = nullptr;
    prev = std::make_shared<StateStorage>(prev);
    auto tableStorage = std::make_shared<StateStorage>(prev);

    BOOST_CHECK(prev->createTable("t_test", valueFields));

    auto table = prev->openTable("t_test");
    BOOST_TEST(table);

    for (size_t i = 0; i < 100; ++i)
    {
        auto entry = table->newEntry();
        entry.importFields({"data" + boost::lexical_cast<std::string>(i), "data2", "data3"});
        table->setRow("key" + boost::lexical_cast<std::string>(i), entry);
    }

    // query 50-150
    std::vector<std::string> keys;
    for (size_t i = 50; i < 150; ++i)
    {
        keys.push_back("key" + boost::lexical_cast<std::string>(i));
    }

    auto queryTable = tableStorage->openTable("t_test");
    BOOST_TEST(queryTable);

    std::vector<std::string_view> views;
    for (auto& key : keys)
    {
        views.push_back(key);
    }
    auto values = queryTable->getRows(views);

    for (size_t i = 0; i < 100; ++i)
    {
        auto entry = values[i];
        if (i + 50 < 100)
        {
            BOOST_TEST(entry);
            BOOST_CHECK_EQUAL(entry->dirty(), false);
            BOOST_CHECK_GT(entry->capacityOfHashField(), 0);
        }
        else
        {
            BOOST_TEST(!entry);
        }
    }

    for (size_t i = 0; i < 10; ++i)
    {
        auto entry = queryTable->newEntry();
        entry.importFields({"data" + boost::lexical_cast<std::string>(i), "data2", "data3"});
        queryTable->setRow("key" + boost::lexical_cast<std::string>(i), entry);
    }

    // Query 0-30 local(0-9) prev(10-29)
    keys.clear();
    for (size_t i = 0; i < 30; ++i)
    {
        keys.push_back("key" + boost::lexical_cast<std::string>(i));
    }

    views.clear();
    for (auto& key : keys)
    {
        views.push_back(key);
    }
    values = queryTable->getRows(views);

    for (size_t i = 0; i < 30; ++i)
    {
        auto entry = values[i];
        if (i < 10)
        {
            BOOST_TEST(entry);
            BOOST_CHECK_EQUAL(entry->dirty(), true);
        }
        else
        {
            BOOST_TEST(entry);
            BOOST_CHECK_EQUAL(entry->dirty(), false);
        }
    }

    // Test deleted entry
    for (size_t i = 10; i < 20; ++i)
    {
        queryTable->setRow(
            "key" + boost::lexical_cast<std::string>(i), queryTable->newDeletedEntry());
    }

    auto values2 = queryTable->getRows(keys);
    for (size_t i = 0; i < values2.size(); ++i)
    {
        if (i >= 10 && i < 20)
        {
            BOOST_CHECK(!values2[i]);
        }
        else
        {
            BOOST_CHECK(values2[i]);
        }
    }

    // Test rollback
    auto recoder = tableStorage->newRecoder();
    tableStorage->setRecoder(recoder);
    for (size_t i = 70; i < 80; ++i)
    {
        Entry myEntry;
        myEntry.importFields({"ddd1", "ddd2", "ddd3"});
        queryTable->setRow("key" + boost::lexical_cast<std::string>(i), std::move(myEntry));
    }

    keys.clear();
    for (size_t i = 70; i < 80; ++i)
    {
        keys.push_back("key" + boost::lexical_cast<std::string>(i));
    }

    auto values3 = queryTable->getRows(keys);
    for (auto& it : values3)
    {
        BOOST_CHECK(it);
        BOOST_CHECK_EQUAL(it->getField(0), "ddd1");
        BOOST_CHECK_EQUAL(it->dirty(), true);
    }

    tableStorage->rollback(recoder);

    auto values4 = queryTable->getRows(keys);
    size_t count = 70;
    for (auto& it : values4)
    {
        BOOST_CHECK(it);
        BOOST_CHECK_EQUAL(it->getField(0), "data" + boost::lexical_cast<std::string>(count));
        BOOST_CHECK_EQUAL(it->dirty(), false);
        ++count;
    }
}

BOOST_AUTO_TEST_CASE(checkVersion)
{
    BOOST_CHECK_NO_THROW(tableFactory->createTable("testTable", "value1, value2, value3"));
    auto table = tableFactory->openTable("testTable");

    Entry value1;
    value1.importFields({"v1"});
    table->setRow("abc", std::move(value1));

    Entry value2;
    value2.importFields({"v2"});
    BOOST_CHECK_NO_THROW(table->setRow("abc", std::move(value2)));

    Entry value3;
    value3.importFields({"v3"});
    BOOST_CHECK_NO_THROW(table->setRow("abc", std::move(value3)));
}

BOOST_AUTO_TEST_CASE(deleteAndGetRows)
{
    StateStorage::Ptr storage1 = std::make_shared<StateStorage>(nullptr);
    storage1->setEnableTraverse(true);

    storage1->asyncCreateTable(
        "table", "value", [](Error::UniquePtr error, std::optional<Table> table) {
            BOOST_CHECK(!error);
            BOOST_CHECK(table);
        });

    Entry entry1;
    entry1.importFields({"value1"});
    storage1->asyncSetRow(
        "table", "key1", std::move(entry1), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    Entry entry2;
    entry2.importFields({"value2"});
    storage1->asyncSetRow(
        "table", "key2", std::move(entry2), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    StateStorage::Ptr storage2 = std::make_shared<StateStorage>(storage1);
    storage2->setEnableTraverse(true);
    Entry deleteEntry;
    deleteEntry.setStatus(Entry::DELETED);
    storage2->asyncSetRow("table", "key2", std::move(deleteEntry),
        [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    StateStorage::Ptr storage3 = std::make_shared<StateStorage>(storage2);
    storage3->asyncGetPrimaryKeys(
        "table", std::nullopt, [](Error::UniquePtr error, std::vector<std::string> keys) {
            BOOST_CHECK(!error);
            BOOST_CHECK_EQUAL(keys.size(), 1);
            BOOST_CHECK_EQUAL(keys[0], "key1");
        });
}

BOOST_AUTO_TEST_CASE(deletedAndGetRow)
{
    StateStorage::Ptr storage1 = std::make_shared<StateStorage>(nullptr);

    storage1->asyncCreateTable(
        "table", "value", [](Error::UniquePtr error, std::optional<Table> table) {
            BOOST_CHECK(!error);
            BOOST_CHECK(table);
        });

    Entry entry1;
    entry1.importFields({"value1"});
    storage1->asyncSetRow(
        "table", "key1", std::move(entry1), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    StateStorage::Ptr storage2 = std::make_shared<StateStorage>(storage1);
    Entry deleteEntry;
    deleteEntry.setStatus(Entry::DELETED);
    storage2->asyncSetRow("table", "key1", std::move(deleteEntry),
        [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    storage2->asyncGetRow("table", "key1", [](Error::UniquePtr error, std::optional<Entry> entry) {
        BOOST_CHECK(!error);
        BOOST_CHECK(!entry);
    });

    storage2->asyncGetRow("table", "key1", [](Error::UniquePtr error, std::optional<Entry> entry) {
        BOOST_CHECK(!error);
        BOOST_CHECK(!entry);
    });
}

BOOST_AUTO_TEST_CASE(deletedAndGetRows)
{
    StateStorage::Ptr storage1 = std::make_shared<StateStorage>(nullptr);

    storage1->asyncCreateTable(
        "table", "value", [](Error::UniquePtr error, std::optional<Table> table) {
            BOOST_CHECK(!error);
            BOOST_CHECK(table);
        });

    Entry entry1;
    entry1.importFields({"value1"});
    storage1->asyncSetRow(
        "table", "key1", std::move(entry1), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    StateStorage::Ptr storage2 = std::make_shared<StateStorage>(storage1);
    Entry deleteEntry;
    deleteEntry.setStatus(Entry::DELETED);
    storage2->asyncSetRow("table", "key1", std::move(deleteEntry),
        [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    std::string_view keys[] = {"key1"};
    storage2->asyncGetRows(
        "table", keys, [](Error::UniquePtr error, std::vector<std::optional<Entry>> entry) {
            BOOST_CHECK(!error);
            BOOST_CHECK_EQUAL(entry.size(), 1);
            BOOST_CHECK(!entry[0]);
        });
}

BOOST_AUTO_TEST_CASE(rollbackAndGetRow)
{
    StateStorage::Ptr storage1 = std::make_shared<StateStorage>(nullptr);

    storage1->asyncCreateTable(
        "table", "value", [](Error::UniquePtr error, std::optional<Table> table) {
            BOOST_CHECK(!error);
            BOOST_CHECK(table);
        });

    Entry entry1;
    entry1.importFields({"value1"});
    storage1->asyncSetRow(
        "table", "key1", std::move(entry1), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    StateStorage::Ptr storage2 = std::make_shared<StateStorage>(storage1);
    auto recoder = storage2->newRecoder();
    storage2->setRecoder(recoder);

    Entry entry2;
    entry2.importFields({"value2"});
    storage2->asyncSetRow(
        "table", "key1", std::move(entry2), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    storage2->asyncGetRow("table", "key1", [](Error::UniquePtr error, std::optional<Entry> entry) {
        BOOST_CHECK(!error);
        BOOST_CHECK(entry);
        BOOST_CHECK_EQUAL(entry->getField(0), "value2");
    });

    storage2->rollback(recoder);

    storage2->asyncGetRow("table", "key1", [](Error::UniquePtr error, std::optional<Entry> entry) {
        BOOST_CHECK(!error);
        BOOST_CHECK(entry);
        BOOST_CHECK_EQUAL(entry->getField(0), "value1");
    });
}

BOOST_AUTO_TEST_CASE(rollbackAndGetRows)
{
    StateStorage::Ptr storage1 = std::make_shared<StateStorage>(nullptr);

    storage1->asyncCreateTable(
        "table", "value", [](Error::UniquePtr error, std::optional<Table> table) {
            BOOST_CHECK(!error);
            BOOST_CHECK(table);
        });

    Entry entry1;
    entry1.importFields({"value1"});
    storage1->asyncSetRow(
        "table", "key1", std::move(entry1), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    StateStorage::Ptr storage2 = std::make_shared<StateStorage>(storage1);
    auto recoder = storage2->newRecoder();
    storage2->setRecoder(recoder);

    Entry entry2;
    entry2.importFields({"value2"});
    storage2->asyncSetRow(
        "table", "key1", std::move(entry2), [](Error::UniquePtr error) { BOOST_CHECK(!error); });

    std::string_view keys[] = {"key1"};
    storage2->asyncGetRows(
        "table", keys, [](Error::UniquePtr error, std::vector<std::optional<Entry>> entry) {
            BOOST_CHECK(!error);
            BOOST_CHECK_EQUAL(entry.size(), 1);
            BOOST_CHECK_EQUAL(entry[0].value().getField(0), "value2");
        });

    storage2->rollback(recoder);

    storage2->asyncGetRows(
        "table", keys, [](Error::UniquePtr error, std::vector<std::optional<Entry>> entry) {
            BOOST_CHECK(!error);
            BOOST_CHECK_EQUAL(entry.size(), 1);
            BOOST_CHECK_EQUAL(entry[0].value().getField(0), "value1");
        });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace bcos::test
*/