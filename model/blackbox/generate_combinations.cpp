#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>

int main() {
    std::vector<std::string> data_sizes = {
        std::to_string(100UL * (1UL << 20)), // 100MB in bytes
        std::to_string(1UL * (1UL << 30)),   // 1GB in bytes
        std::to_string(10UL * (1UL << 30))  // 10GB in bytes
    };
    std::vector<std::string> write_buffer_sizes = {"2M"};
    std::vector<std::string> block_cache_sizes = {"64M", "128M"};
    std::vector<std::string> compaction_styles = {"level"};
    std::vector<std::string> bloom_filter_policies = {"true"};
    std::vector<std::string> operation_types = {"PUT", "GET", "SEEK"};
    std::vector<std::string> number_of_operations = {"100", "1000", "10000", "100000"};

    std::vector<std::vector<std::string>> params = {
        data_sizes,
        operation_types,
        write_buffer_sizes,
        block_cache_sizes,
        compaction_styles,
        bloom_filter_policies,
        number_of_operations
    };

    std::set<std::vector<std::string>> combinations;
    std::vector<int> indices(params.size(), 0);

    while (true) {
        std::vector<std::string> current_combination;
        for (int i = 0; i < params.size(); ++i) {
            current_combination.push_back(params[i][indices[i]]);
        }
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

    std::ofstream outfile("combinations.csv");
    if (!outfile.is_open()) {
        std::cerr << "Error opening combinations.csv" << std::endl;
        return 1;
    }

    outfile << "data_size,operation_type,write_buffer_size,block_cache_size,compaction_style,bloom_filter_policy,number_of_operations\n";
    for (const auto& combination : combinations) {
        for (size_t i = 0; i < combination.size(); ++i) {
            outfile << combination[i] << (i == combination.size() - 1 ? "" : ",");
        }
        outfile << "\n";
    }

    outfile.close();
    return 0;
}