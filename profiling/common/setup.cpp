#include <rocksdb/db.h>
#include <iostream>
#include <vector>

int main() {
    rocksdb::DB* db;
    rocksdb::Options options;
    // options.create_if_missing = true;
    // options.IncreaseParallelism();  // Adjust parallelism if needed
    // options.OptimizeLevelStyleCompaction();  // Example optimization
    // std::cout.setf( std::ios_base::unitbuf ); // cause a flush at the end of each print

    // Attempt to open the database
    rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);
    if (!status.ok()) {
        std::cerr << "Error opening database: " << status.ToString() << std::endl;
        return 1; // Exit if DB cannot be opened
    }
    std::cout << "Database opened successfully." << std::endl;

    // Insert a large number of key-value pairs
    for (int i = 0; i < 100; ++i) {
        status = db->Put(rocksdb::WriteOptions(), "key" + std::to_string(i), "value" + std::to_string(i));
        if (!status.ok()) {
            std::cerr << "Error writing key" << i << ": " << status.ToString() << std::endl;
            return 1; // Exit if any write operation fails
        }
    }
    std::cout << "Database populated with 100 key-value pairs." << std::endl;

    // Attempt to close the database
    status = db -> Close();
    if (!status.ok()) {
        std::cerr << "Error closing database: " << status.ToString() << std::endl;
        return 1; // Exit if DB cannot be closed properly
    }
    std::cout << "Database closed successfully." << std::endl;

    return 0;
}
