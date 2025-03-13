#include <rocksdb/db.h>
#include <iostream>
#include <vector>
#include <unistd.h>

int main() {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    std::cout.setf( std::ios_base::unitbuf ); // cause a flush at the end of each print

    // Open the database
    rocksdb::Status status = rocksdb::DB::Open(options, "/var/tmp/testdb", &db);
    if (!status.ok()) {
        throw std::invalid_argument("Error opening database");
    }

    std::string value;
    // Perform lookups for existing keys
    
    sleep(2);
    status = db->Get(rocksdb::ReadOptions(), "key1", &value);
    sleep(2);
    if (!status.ok()) {
        throw std::invalid_argument("Error retrieving value");
    }

  // Perform a lookup for a non-existent key (this should take more time)
    // std::cout << "BEGIN_POINT_LOOKUP: Key !Exists" << std::endl;
    // status = db->Get(rocksdb::ReadOptions(), "key_non_existent", &value);
    // if (!status.IsNotFound()) {
    //     throw std::invalid_argument("Found !existing key");
    // }
    // std::cout << "END_POINT_LOOKUP: Key !Exists" << std::endl;
    // std::cout << "Retrieved: " << value << std::endl;

    // Close the database
    status = db -> Close();
    if (!status.ok()) {
        throw std::invalid_argument("Error closing database");
    }
    return 0;
}
