#include "aocli.hh"
#include <iostream>
#include <iterator>
#include <string>
#include <fstream>

std::string get_cached_input(const Config& config, int year, int day) {
    // Construct input file path
    fs::path input_file = config.inputs_dir /
                         (std::to_string(year) + "_" +
                          std::to_string(day) + ".txt");

    if (fs::exists(input_file)) {
        // Open file in binary mode and seek to end for size
        std::ifstream file(input_file, std::ios::binary | std::ios::ate);
        if (!file) {
            return "";
        }

        // Get file size and reserve string capacity
        auto size = file.tellg();
        if (size == -1) {
            return "";
        }

        // Pre-allocate string and read entire file
        std::string content;
        content.reserve(size);
        file.seekg(0);

        // Read entire file at once using iterators
        content.assign(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        return content;
    }

    return "";
}

void cache_input(const Config& config, int year, int day,
                const std::string& input) {
    // Construct input file path
    fs::path input_file = config.inputs_dir /
                         (std::to_string(year) + "_" +
                          std::to_string(day) + ".txt");

    // Open file in binary mode for better performance
    std::ofstream file(input_file, std::ios::binary);
    if (!file) {
        return;
    }

    // Set up buffering for better performance
    static constexpr size_t BUFFER_SIZE = 8192;  // 8KB buffer
    char buffer[BUFFER_SIZE];
    file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

    // Write entire content at once
    file.write(input.data(), input.size());
}
