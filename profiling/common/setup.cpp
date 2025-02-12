#include <rocksdb/db.h>
#include <iostream>
#include <vector>

int main() {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    std::cout.setf( std::ios_base::unitbuf ); // cause a flush at the end of each print

    // Open the database
    rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);
    if (!status.ok()) {
        throw std::invalid_argument("Error opening database");
    }

    // Insert a large number of key-value pairs
    for (int i = 0; i < 100; ++i) {
        status = db->Put(rocksdb::WriteOptions(), "key" + std::to_string(i), "value" + std::to_string(i));
        if (!status.ok()) {
            throw std::invalid_argument("Error writing value");
        }
    }

    status = db -> Close();
    if (!status.ok()) {
        throw std::invalid_argument("Error closing database");
    }
    return 0;
}
