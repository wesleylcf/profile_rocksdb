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
#include <csignal>
#include <cstdlib> // For getenv

using namespace std;
using namespace rocksdb;
using namespace std::chrono;
namespace fs = std::filesystem;

string DB_PATH = "/tmp/testdb";
string OUTPUT_PATH; // Dynamically calculated
size_t KEY_SIZE = 28;
size_t VALUE_SIZE = 100;
size_t ENTRY_SIZE = KEY_SIZE + VALUE_SIZE;
uint64_t MAX_BYTES_PER_LEVEL_BASE = 20 * (1UL << 20); // Fixed 20MB max_bytes_for_level_base
int BLOOM_FILTER_BITS_PER_KEY = 10;

struct BenchmarkResult {
    size_t data_size;
    string operation_type;
    string write_buffer_size;
    string compaction_style;
    string bloom_filter_policy;
    size_t number_of_operations;
    long long latency;
};

string generate_uuid() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return string(uuid_str);
}

string generate_fixed_size_key(uint64_t index, size_t key_size) {
    string key;
    key.resize(key_size);
    size_t index_bytes = min(key_size, (size_t)8);
    for (size_t i = 0; i < index_bytes; ++i) {
        key[key_size - index_bytes + i] = (index >> (8 * (index_bytes - 1 - i))) & 0xFF;
    }
    for (size_t i = 0; i < key_size - index_bytes; ++i) {
        key[i] = 0;
    }
    return key;
}

string generate_random_string(size_t length) {
    string str(length, 0);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(33, 126);
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
            break;
    }
    return size;
}

vector<BenchmarkResult> run_benchmark(size_t num_entries, size_t number_of_operations, const string& write_buffer_size_str, const string& compaction_style, const string& bloom_filter_policy_str, const string& operation_type) {
    vector<BenchmarkResult> results;
    Options options;
    options.create_if_missing = true;
    options.write_buffer_size = parse_size_string(write_buffer_size_str);
    options.max_bytes_for_level_base = MAX_BYTES_PER_LEVEL_BASE;
    options.level_compaction_dynamic_level_bytes = false;
    if (compaction_style == "level") {
        options.compaction_style = kCompactionStyleLevel;
    } else if (compaction_style == "universal") {
        options.compaction_style = kCompactionStyleUniversal;
    } else {
        cerr << "Invalid compaction style: " << compaction_style << endl;
        exit(1);
    }
    BlockBasedTableOptions table_options;
    if (bloom_filter_policy_str == "true") {
        table_options.filter_policy.reset(NewBloomFilterPolicy(BLOOM_FILTER_BITS_PER_KEY));
    }
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    string db_path = DB_PATH + "_" + generate_uuid();
    cout << "Log: Opening RocksDB at " << db_path << endl;
    DB* db;
    Status s = DB::Open(options, db_path, &db);
    string value = generate_random_string(VALUE_SIZE);
    long long duration = 0;
    if (!s.ok()) {
        cerr << "Error opening database: " << s.ToString() << endl;
        exit(1);
    }
    unordered_set<int> sample_indices;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> distrib(0, num_entries - 1);
    while (sample_indices.size() < number_of_operations) {
        sample_indices.insert(distrib(gen));
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
        if (sample_indices.find(i) != sample_indices.end()) {
            duration = duration_cast<microseconds>(stop - start).count();
            results.push_back({num_entries * ENTRY_SIZE, operation_type, write_buffer_size_str, compaction_style, bloom_filter_policy_str, number_of_operations, duration});
        }
    }
    if (operation_type == "GET") {
        results.clear();
        vector<string> random_keys;
        for (int idx : sample_indices) {
            random_keys.push_back(generate_fixed_size_key(idx, KEY_SIZE));
        }
        for (const string& key : random_keys) {
            string read_value;
            auto start = high_resolution_clock::now();
            s = db->Get(ReadOptions(), key, &read_value);
            auto stop = high_resolution_clock::now();
            duration = duration_cast<microseconds>(stop - start).count();
            results.push_back({num_entries * ENTRY_SIZE, operation_type, write_buffer_size_str, compaction_style, bloom_filter_policy_str, number_of_operations, duration});
            if (!s.ok()) {
                cerr << "Error during GET: " << s.ToString() << endl;
                exit(1);
            }
        }
    } else if (operation_type == "SEEK") {
        results.clear();
        vector<string> last_keys;
        for (size_t i = num_entries - number_of_operations; i < num_entries; ++i) {
            last_keys.push_back(generate_fixed_size_key(i, KEY_SIZE));
        }
        for (const string& key : last_keys) {
            auto start = high_resolution_clock::now();
            Iterator* iter = db->NewIterator(ReadOptions());
            iter->Seek(key);
            auto stop = high_resolution_clock::now();
            delete iter;
            duration = duration_cast<microseconds>(stop - start).count();
            results.push_back({num_entries * ENTRY_SIZE, operation_type, write_buffer_size_str, compaction_style, bloom_filter_policy_str, number_of_operations, duration});
        }
    }

    delete db;
    cout << "Log: Closing RocksDB at " << db_path << endl;

    try {
        fs::remove_all(db_path);
        cout << "Log: Removed RocksDB directory " << db_path << endl;
    } catch (const exception& e) {
        cerr << "Error removing database directory: " << e.what() << endl;
    }

    return results;
}

void remove_directories_with_prefix(const std::string& parent_path, const std::string& prefix) {
    try {
        for (const auto& entry : fs::directory_iterator(parent_path)) {
            if (fs::is_directory(entry.status())) {
                std::string entry_path = entry.path().string();
                if (entry_path.rfind(prefix, 0) == 0) {
                    try {
                        fs::remove_all(entry.path());
                        cout << "Log: Removed directory " << entry_path << endl;
                    } catch (const fs::filesystem_error& remove_ex) {
                        std::cerr << "Remove error: " << remove_ex.what() << std::endl;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& ex) {
        std::cerr << "Filesystem error: " << ex.what() << std::endl;
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "Interrupt signal (Ctrl+C) received. Cleaning up..." << std::endl;

        try {
            remove_directories_with_prefix("/tmp", "/tmp/testdb");
        } catch (const exception& e) {
            cerr << "Error cleaning up database directory: " << e.what() << endl;
        }
        exit(signal);
    }
}

int main() {
    try {
        std::signal(SIGINT, signalHandler);

        // Dynamically calculate OUTPUT_PATH
        char* project_dir = getenv("PWD"); // Get current working directory
        if (project_dir) {
            OUTPUT_PATH = string(project_dir) + "/model/blackbox/raw.csv";
            cout << "Log: Output path set to: " << OUTPUT_PATH << endl;
        } else {
            cerr << "Error: PWD environment variable not set. Using default output path." << endl;
            OUTPUT_PATH = "raw.csv"; // Default path
            cout << "Log: Default output path used: " << OUTPUT_PATH << endl;
        }

        vector<string> data_sizes = {
            to_string(100UL * (1UL << 20)),
            to_string(1UL * (1UL << 30)),
            to_string(10UL * (1UL << 30))
        };
        vector<string> write_buffer_sizes = {"2M"};
        vector<string> compaction_styles = {"level"};
        vector<string> bloom_filter_policies = {"true"};
        vector<string> operation_types = {"PUT", "GET", "SEEK"};
        vector<string> number_of_operations = {"100", "1000", "10000", "100000"};

        vector<vector<string>> params = {
            data_sizes,
            operation_types,
            write_buffer_sizes,
            compaction_styles,
            bloom_filter_policies,
            number_of_operations
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

            if (k < 0) break;
        }

        ofstream outfile(OUTPUT_PATH);
        outfile << "data_size,operation_type,write_buffer_size,compaction_style,bloom_filter_policy,number_of_operations,latency\n";

        auto overall_start = high_resolution_clock::now();
        int total_runs = combinations.size();
        int completed_runs = 0;

        for (const auto& combination : combinations) {
            size_t data_size = stoul(combination[0]);
            size_t num_entries = data_size / ENTRY_SIZE;
            string operation_type = combination[1];
            string write_buffer_size = combination[2];
            string compaction_style = combination[3];
            string bloom_filter_policy = combination[4];
            size_t number_of_operations = stoul(combination[5]);

            cout << "Log: Starting benchmark with parameters: "
                << "data_size=" << data_size << ", "
                << "operation_type=" << operation_type << ", "
                << "write_buffer_size=" << write_buffer_size << ", "
                << "compaction_style=" << compaction_style << ", "
                << "bloom_filter_policy=" << bloom_filter_policy << ", "
                << "number_of_operations=" << number_of_operations << endl;

            vector<BenchmarkResult> results = run_benchmark(num_entries, number_of_operations, write_buffer_size, compaction_style, bloom_filter_policy, operation_type);

            for (const auto& result : results) {
                outfile << result.data_size << "," << result.operation_type << ","
                        << result.write_buffer_size << ","
                        << result.compaction_style << "," << result.bloom_filter_policy << ","
                        << result.number_of_operations << "," << result.latency << "\n";
            }

            completed_runs++;

            cout << "\033[2K\rCompleted job " << completed_runs << "/" << total_runs << " with parameters: "
                << "data_size=" << data_size << ", "
                << "operation_type=" << operation_type << ", "
                << "write_buffer_size=" << write_buffer_size << ", "
                << "compaction_style=" << compaction_style << ", "
                << "bloom_filter_policy=" << bloom_filter_policy << ", "
                << "number_of_operations=" << number_of_operations << endl;

            if (completed_runs < total_runs) {
                cout << "\033[2K\rCurrent job " << (completed_runs + 1) << "/" << total_runs << " ...\n" << flush;
            }
        }

        cout << endl;
        outfile.close();
        cout << "Log: Benchmark completed. Results written to: " << OUTPUT_PATH << endl;
        return 0;
    } catch (const exception& e) {
        cerr << "Error in main(): " << e.what() << endl;
        remove_directories_with_prefix("/tmp", "/tmp/testdb");
    }
}