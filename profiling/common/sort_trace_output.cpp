#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

// Function to parse a line from the trace output and extract time elapsed
uint64_t extractTimeElapsed(const std::string& line) {
    if (line.empty()) {
        return 0;  // Return 0 for empty lines
    }


    // Split the line into space-separated tokens
    std::istringstream stream(line);
    std::string word;

    std::string last_word, second_last_word, third_last_word;
    while (stream >> word) {
        third_last_word = second_last_word;
        second_last_word = last_word;
        last_word = word;
    }


    // If the line has the format "took <time> ns", extract the time
    if (third_last_word == "took" && last_word == "ns") {
        uint64_t time_elapsed = std::stoull(second_last_word);  // Convert to uint64_t
        return time_elapsed;
    }

    return 0;  // Return 0 if time elapsed is not found
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <absolute_file_path>" << std::endl;
        return 1;
    }

    std::string input_file_path = argv[1];
    std::ifstream input_file(input_file_path);

    if (!input_file.is_open()) {
        std::cerr << "Error opening file: " << input_file_path << std::endl;
        return 1;
    }

    // Create the output file path as a sibling to the input file
    std::filesystem::path input_path(input_file_path);
    std::filesystem::path output_file_path = input_path.parent_path() / "time_elapsed.csv";

    std::ofstream output_file(output_file_path);

    if (!output_file.is_open()) {
        std::cerr << "Error creating output file: " << output_file_path << std::endl;
        return 1;
    }

    std::vector<std::pair<std::string, uint64_t>> rows;
    std::string line;

    // Read lines from the input file
    bool is_header = true;
    while (std::getline(input_file, line)) {
        // Skip empty lines and the header row
        if (line.empty() || is_header) {
            is_header = false;
            continue;
        }

        uint64_t time_elapsed = extractTimeElapsed(line);

        // Only store rows with valid time elapsed
        if (time_elapsed > 0) {
            rows.push_back({line, time_elapsed});
        }
    }

    // Sort the rows by time elapsed in descending order
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Write the sorted result to the output CSV file
    output_file << "Row,Time Elapsed (ns)" << std::endl;
    for (const auto& row : rows) {
        output_file << "\"" << row.first << "\"," << row.second << std::endl;
    }


    return 0;
}
