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
string OUTPUT_PATH = "/Users/wesley/Documents/GitHub/rocksdb/model/blackbox/rocksdb_benchmark_results1.csv";

struct BenchmarkResult {
    size_t data_size;
    string operation_type;
    string write_buffer_size;
    string block_cache_size;
    string compaction_style;
    int max_background_jobs;
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
    std::istringstream iss(size_str);
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

BenchmarkResult run_benchmark(size_t num_entries, const string& write_buffer_size_str, const string& block_cache_size_str, const string& compaction_style, int max_background_jobs, const string& bloom_filter_policy_str, const string& operation_type) {
    Options options;
    options.create_if_missing = true;
    options.write_buffer_size = parse_size_string(write_buffer_size_str);

    if (compaction_style == "level") {
        options.compaction_style = kCompactionStyleLevel;
    } else if (compaction_style == "universal") {
        options.compaction_style = kCompactionStyleUniversal;
    } else {
        cerr << "Invalid compaction style: " << compaction_style << endl;
        exit(1);
    }

    options.max_background_jobs = max_background_jobs;

    // Block Cache and Bloom Filter (Correct Way - as per RocksDB documentation)
    BlockBasedTableOptions table_options;

    table_options.block_cache = NewLRUCache(parse_size_string(block_cache_size_str));

    if (bloom_filter_policy_str == "true") {
      table_options.filter_policy.reset(NewBloomFilterPolicy(10));
    }

    options.table_factory.reset(NewBlockBasedTableFactory(table_options));


    string db_path = DB_PATH + "_" + generate_uuid();

    DB* db;
    Status s = DB::Open(options, db_path, &db);

    string value = generate_random_string(100);
    long long put_duration_sum = 0;

    if (!s.ok()) {
        cerr << "Error opening database: " << s.ToString() << endl;
        exit(1);
    }

    for (size_t i = 0; i < num_entries; ++i) {
        string key = "key_" + to_string(i);
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
            random_keys.push_back("key_" + to_string(idx));
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
            last_keys.push_back("key_" + to_string(i));
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
    } catch (const std::exception& e) {
        cerr << "Error removing database directory: " << e.what() << endl;
        // Handle the error appropriately, perhaps exit or log it.
    }

    return {num_entries, operation_type, write_buffer_size_str, block_cache_size_str, compaction_style, max_background_jobs, bloom_filter_policy_str, duration};
}


int main() {
    try {
        fs::remove_all(DB_PATH); // Remove the base DB directory
    } catch (const std::exception& e) {
        cerr << "Error cleaning up database directory: " << e.what() << endl;
        return 1; // Or handle the error appropriately
    }
  
    vector<string> num_entries = {"10000", "50000", "100000", "200000", "500000", "1000000"};
    vector<string> write_buffer_sizes = {"64M", "128M"};
    vector<string> block_cache_sizes = {"64M", "128M"};
    // vector<string> compaction_styles = {"level", "universal"};
    // vector<string> max_background_jobs_values = {"2", "4"};
    // vector<string> bloom_filter_policies = {"true", "false"};
    // vector<string> operation_types = {"PUT", "GET", "SEEK"};
      // vector<string> num_entries = {"10000"};
      // vector<string> write_buffer_sizes = {"128M"};
      // vector<string> block_cache_sizes = {"128M"};
      vector<string> compaction_styles = {"level", "universal"};
      vector<string> max_background_jobs_values = {"2", "4"};
      vector<string> bloom_filter_policies = {"true", "false"};
      vector<string> operation_types = {"GET"};


    vector<vector<string>> params = {
        num_entries,      // Convert size_t to string
        operation_types,
        write_buffer_sizes,
        block_cache_sizes,
        compaction_styles,
        max_background_jobs_values,
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
            params[5][indices[5]],
            params[6][indices[6]]
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

    int total_runs = combinations.size();
    ThreadSafeCounter completed_runs;
    mutex progress_mutex;

    ofstream outfile(OUTPUT_PATH);
    outfile << "data_size,operation_type,write_buffer_size,block_cache_size,compaction_style,max_background_jobs,bloom_filter_policy,latency\n";

    auto overall_start = high_resolution_clock::now();
    vector<future<BenchmarkResult>> futures;
    std::counting_semaphore max_concurrency(std::thread::hardware_concurrency()); // Limit concurrency

    for (const auto& combination : combinations) {
        size_t data_size = stoul(combination[0]);
        string operation_type = combination[1];
        string write_buffer_size = combination[2];
        string block_cache_size = combination[3];
        string compaction_style = combination[4];
        int max_background_jobs = stoi(combination[5]);
        string bloom_filter_policy = combination[6];

        futures.push_back(async(launch::async, [&, data_size, operation_type, write_buffer_size, block_cache_size, compaction_style, max_background_jobs, bloom_filter_policy](){
            BenchmarkResult result = run_benchmark(data_size, write_buffer_size, block_cache_size, compaction_style, max_background_jobs, bloom_filter_policy, operation_type);
            return result;
        }));
    }

    for (auto& future : futures) {
        try {
            BenchmarkResult result = future.get();

            outfile << result.data_size << "," << result.operation_type << ","
                    << result.write_buffer_size << "," << result.block_cache_size << ","
                    << result.compaction_style << "," << result.max_background_jobs << ","
                    << result.bloom_filter_policy << "," << result.latency << "\n";

            {
                lock_guard<mutex> lock(progress_mutex);
                completed_runs.increment();
                auto now = high_resolution_clock::now();
                auto elapsed_time = duration_cast<seconds>(now - overall_start).count();

                double percentage_complete = (double)completed_runs.get_value() / total_runs * 100.0;

                // Correct estimated time calculation:
                double estimated_total_time = (percentage_complete > 0) ? (elapsed_time / percentage_complete * 100.0) : 0.0;
                double estimated_time_remaining = estimated_total_time - elapsed_time;

                cout << fixed << setprecision(2) << "Progress: " << percentage_complete << "% | Elapsed: " << elapsed_time << "s | Remaining: " << estimated_time_remaining << "s | Config: ";

                // Print the config for the completed benchmark
                cout << result.data_size << "," << result.operation_type << ","
                     << result.write_buffer_size << "," << result.block_cache_size << ","
                     << result.compaction_style << "," << result.max_background_jobs << ","
                     << result.bloom_filter_policy << "\r";

            }
        } catch (const std::future_error& e) {
            cerr << "Future error: " << e.what() << endl;
        } catch (const std::exception& e) {
            cerr << "Exception in main thread (result retrieval): " << e.what() << endl;
        } catch (...) {
            cerr << "Unknown exception in main thread (result retrieval)" << endl;
        }
    }
    futures.clear();

    cout << endl;
    outfile.close();
    return 0;
}
