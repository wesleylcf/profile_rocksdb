#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/cache.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <cmath>
#include <sstream>
#include <cstdint>
#include <thread>
#include <mutex>
#include <future>
#include <iomanip>
#include <filesystem>
#include <tuple>
#include <set>
#include <semaphore>
#include <uuid/uuid.h> 
#include "../common/thread_safe_counter.h"

using namespace std;
using namespace rocksdb;
using namespace std::chrono;
namespace fs = std::filesystem;

string DB_PATH = "/tmp/testdb";
string OUTPUT_PATH = "/Users/wesley/Documents/GitHub/rocksdb/model/blackbox/rocksdb_benchmark_results_sequential.csv";
size_t KEY_SIZE = 10;
size_t VALUE_SIZE = 90;
size_t KEY_VALUE_SIZE = 100;
uint64_t MAX_BYTES_PER_LEVEL_BASE = 20 * (1UL << 20); // Fixed 20MB max_bytes_for_level_base
int BLOOM_FILTER_BITS_PER_KEY = 10;

struct BenchmarkResult {
    size_t data_size;
    string operation_type;
    string write_buffer_size;
    string block_cache_size;
    string compaction_style;
    string bloom_filter_policy;
    long long latency;
};

string generate_uuid() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return string(uuid_str);
}

/*
 *  Generates a key fixed to key_size bytes based on index, ensuring insertion order is maintained
 */
string generate_fixed_size_key(uint64_t index, size_t key_size) {
    string key;
    key.resize(key_size); // Allocate key_size bytes

    // Calculate how many bytes are needed to represent the index
    size_t index_bytes = min(key_size, (size_t)8); // Limit to 8 bytes for uint64_t

    // Convert index to big-endian binary representation
    for (size_t i = 0; i < index_bytes; ++i) {
        key[key_size - index_bytes + i] = (index >> (8 * (index_bytes - 1 - i))) & 0xFF;
    }

    // Pad remaining bytes with zeros
    for (size_t i = 0; i < key_size - index_bytes; ++i) {
        key[i] = 0;
    }

    return key;
}

string generate_random_string(size_t length) {
    string str(length, 0);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(33, 126); // Printable ASCII characters
    for (size_t i = 0; i < length; ++i) {
        str[i] = dis(gen);
    }
    return str;
}

size_t parse_size_string(const string& size_str) {
    istringstream iss(size_str);
    size_t size;
    char unit;
    iss >> size >> unit;

    unit = toupper(unit);

    switch (unit) {
        case 'K': size *= 1024; break;
        case 'M': size *= 1024 * 1024; break;
        case 'G': size *= 1024 * 1024 * 1024; break;
        default:
            if (!isdigit(unit)) {
                cerr << "Invalid size unit: " << unit << endl;
                exit(1);
            }
            break; // No unit specified, assume bytes
    }
    return size;
}

BenchmarkResult run_benchmark(size_t num_entries, const string& write_buffer_size_str, const string& block_cache_size_str, const string& compaction_style, const string& bloom_filter_policy_str, const string& operation_type) {
    Options options;
    options.create_if_missing = true;
    options.write_buffer_size = parse_size_string(write_buffer_size_str);
    options.max_bytes_for_level_base = MAX_BYTES_PER_LEVEL_BASE;
    options.level_compaction_dynamic_level_bytes = false; // Disable dynamic level bytes


    if (compaction_style == "level") {
        options.compaction_style = kCompactionStyleLevel;
    } else if (compaction_style == "universal") {
        options.compaction_style = kCompactionStyleUniversal;
    } else {
        cerr << "Invalid compaction style: " << compaction_style << endl;
        exit(1);
    }


    // Block Cache and Bloom Filter
    BlockBasedTableOptions table_options;
    table_options.block_cache = NewLRUCache(parse_size_string(block_cache_size_str));

    if (bloom_filter_policy_str == "true") {
        table_options.filter_policy.reset(NewBloomFilterPolicy(BLOOM_FILTER_BITS_PER_KEY));
    }

    options.table_factory.reset(NewBlockBasedTableFactory(table_options));

    string db_path = DB_PATH + "_" + generate_uuid();

    DB* db;
    Status s = DB::Open(options, db_path, &db);

    string value = generate_random_string(VALUE_SIZE);
    long long put_duration_sum = 0;

    if (!s.ok()) {
        cerr << "Error opening database: " << s.ToString() << endl;
        exit(1);
    }

    for (size_t i = 0; i < num_entries; ++i) {
        string key = generate_fixed_size_key(i, KEY_SIZE);
        auto start = high_resolution_clock::now();
        s = db->Put(WriteOptions(), key, value);
        auto stop = high_resolution_clock::now();

        if (!s.ok()) {
            cerr << "Error during PUT: " << s.ToString() << endl;
            exit(1);
        }
        put_duration_sum += duration_cast<microseconds>(stop - start).count();
    }

    long long duration = 0;
    if (operation_type == "PUT") {
        duration = (num_entries > 0) ? put_duration_sum / num_entries : 0; // Average PUT latency
    } else if (operation_type == "GET") {
        int num_keys_to_get = min(100, (int) num_entries); // Get up to 100 keys
        vector<string> random_keys;
        
        // Generate 100 random keys within the inserted range
        unordered_set<int> unique_indices;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> distrib(0, num_entries - 1);

        while (unique_indices.size() < num_keys_to_get) {
            unique_indices.insert(distrib(gen));
        }

        for (int idx : unique_indices) {
            random_keys.push_back(generate_fixed_size_key(idx, KEY_SIZE));
        }

        long long get_duration_sum = 0;
        for (const string& key : random_keys) {
            string read_value;
            auto start = high_resolution_clock::now();
            s = db->Get(ReadOptions(), key, &read_value);
            auto stop = high_resolution_clock::now();

            if (!s.ok()) {
                cerr << "Error during GET: " << s.ToString() << endl;
                exit(1);
            }

            get_duration_sum += duration_cast<microseconds>(stop - start).count();
        }

        duration = (num_keys_to_get > 0) ? get_duration_sum / num_keys_to_get : 0; // Average GET latency
    } else if (operation_type == "SEEK") {
        int num_keys_to_seek = min((size_t)100, num_entries);
        vector<string> last_keys;
        for (size_t i = num_entries - num_keys_to_seek; i < num_entries; ++i) {
            last_keys.push_back(generate_fixed_size_key(i, KEY_SIZE));
        }

        long long seek_duration_sum = 0;
        for (const string& key : last_keys) {
            auto start = high_resolution_clock::now();
            Iterator* iter = db->NewIterator(ReadOptions());
            iter->Seek(key);
            delete iter;
            auto stop = high_resolution_clock::now();
            seek_duration_sum += duration_cast<microseconds>(stop - start).count();
        }
        duration = (num_keys_to_seek > 0) ? seek_duration_sum / num_keys_to_seek : 0; // Average SEEK latency
    }

    delete db;

    // Remove the database directory after the benchmark is complete
    try {
        fs::remove_all(db_path);
    } catch (const exception& e) {
        cerr << "Error removing database directory: " << e.what() << endl;
    }

    return {num_entries * KEY_VALUE_SIZE, operation_type, write_buffer_size_str, block_cache_size_str, compaction_style, bloom_filter_policy_str, duration};
}

// Function to animate the polling log
void animate_polling_log(int* current_job, int total_jobs, atomic<bool>& running) {
    int dot_count = 0;
    while (running) {
        string dots(dot_count + 1, '.');
        cout << "\033[2K\rCurrent job " << (*current_job)+1 << "/" << total_jobs << dots << flush; // Clear line and rewrite
        dot_count = (dot_count + 1) % 3; // Cycle through 0, 1, 2 for ., .., ...
        this_thread::sleep_for(milliseconds(500)); // Update every 500ms
    }
}

int main() {
    try {
        fs::remove_all(DB_PATH); // Remove the base DB directory
    } catch (const exception& e) {
        cerr << "Error cleaning up database directory: " << e.what() << endl;
        return 1;
    }

    vector<string> data_sizes = {
        to_string(100UL * (1UL << 20)), // 100MB in bytes
        to_string(1UL * (1UL << 30)),   // 1GB in bytes
        to_string(10UL * (1UL << 30))  // 10GB in bytes
    };
    // vector<string> num_entries = {"10000", "50000", "100000", "200000", "500000", "1000000"};
    vector<string> write_buffer_sizes = {"2M"};
    vector<string> block_cache_sizes = {"64M", "128M"};
    vector<string> compaction_styles = {"level"};
    vector<string> bloom_filter_policies = {"true"};
    vector<string> operation_types = {"PUT", "GET", "SEEK"};

    vector<vector<string>> params = {
        data_sizes,
        operation_types,
        write_buffer_sizes,
        block_cache_sizes,
        compaction_styles,
        bloom_filter_policies
    };

    set<vector<string>> combinations;
    vector<int> indices(params.size(), 0);

    while (true) {
        vector<string> current_combination {
            params[0][indices[0]],
            params[1][indices[1]],
            params[2][indices[2]],
            params[3][indices[3]],
            params[4][indices[4]],
            params[5][indices[5]]
        };
        combinations.insert(current_combination);

        int k = params.size() - 1;
        while (k >= 0) {
            indices[k]++;
            if (indices[k] < params[k].size()) {
                break;
            } else {
                indices[k] = 0;
                k--;
            }
        }

        if (k < 0) break; // All combinations generated
    }

    ofstream outfile(OUTPUT_PATH);
    outfile << "data_size,operation_type,write_buffer_size,block_cache_size,compaction_style,bloom_filter_policy,latency\n";

    auto overall_start = high_resolution_clock::now();
    int total_runs = combinations.size();
    int* completed_runs = 0;

    // Atomic flag to control the polling log thread
    atomic<bool> running(true);

    // Start the polling log thread
    thread polling_thread(animate_polling_log, &completed_runs, total_runs, ref(running));

    for (const auto& combination : combinations) {
        size_t data_size = stoul(combination[0]);
        size_t num_entries = data_size / KEY_VALUE_SIZE;
        string operation_type = combination[1];
        string write_buffer_size = combination[2];
        string block_cache_size = combination[3];
        string compaction_style = combination[4];
        string bloom_filter_policy = combination[5];

        BenchmarkResult result = run_benchmark(num_entries, write_buffer_size, block_cache_size, compaction_style, bloom_filter_policy, operation_type);

        outfile << result.data_size << "," << result.operation_type << ","
                << result.write_buffer_size << "," << result.block_cache_size << ","
                << result.compaction_style << ","
                << result.bloom_filter_policy << "," << result.latency << "\n";

        completed_runs++;

        // Log the completion of the current job
        cout << "\033[2K\rCompleted job " << completed_runs << "/" << total_runs << " with parameters: "
             << "data_size=" << data_size << ", "
             << "operation_type=" << operation_type << ", "
             << "write_buffer_size=" << write_buffer_size << ", "
             << "block_cache_size=" << block_cache_size << ", "
             << "compaction_style=" << compaction_style << ", "
             << "bloom_filter_policy=" << bloom_filter_policy << endl;

        // Update the polling log with the next job
        if (*completed_runs < total_runs) {
            cout << "\033[2K\rCurrent job " << (completed_runs + 1) << "/" << total_runs << " ..." << flush;
        }
    }

    // Stop the polling log thread
    running = false;
    polling_thread.join();

    cout << endl;
    outfile.close();
    return 0;
}