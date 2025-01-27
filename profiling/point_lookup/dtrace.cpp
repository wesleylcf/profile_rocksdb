#include <rocksdb/db.h>
#include <iostream>
#include <vector>

int main() {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;

    // Open the database
    rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());

    // Insert a large number of key-value pairs
    for (int i = 0; i < 100; ++i) {
        db->Put(rocksdb::WriteOptions(), "key" + std::to_string(i), "value" + std::to_string(i));
    }
    // for (int i = 0; i < 100000; ++i) {
    //     db->Put(rocksdb::WriteOptions(), "key" + std::to_string(i), "value" + std::to_string(i));
    // }

    // Perform lookups for existing keys
    std::string value;
    db->Get(rocksdb::ReadOptions(), "key1", &value);
    // std::cout << "Retrieved: " << value << std::endl;

    // Perform a lookup for a non-existent key (this should take more time)
    std::cout << "BEGIN_POINT_LOOKUP" << std::endl;
    status = db->Get(rocksdb::ReadOptions(), "key_non_existent", &value);
    std::cout << "END_POINT_LOOKUP" << std::endl;
    std::cout << "Retrieved: " << value << std::endl;

    // Close the database
    delete db;
    return 0;
}
