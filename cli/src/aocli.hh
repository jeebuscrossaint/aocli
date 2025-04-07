#pragma once
#ifndef AOCLI_HH
#define AOCLI_HH

#include <string>
#include <string_view>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <mutex>
#include <optional>
#include <array>
#include <unordered_map>
#include <thread>
#include <future>
#include <fcntl.h>
#include <sys/mman.h>

#include <curl/curl.h>
#include <gumbo.h>

namespace fs = std::filesystem;

// Command line interface utilities
namespace cmd {
    static constexpr std::array commands = {
        std::string_view("fetch"),
        std::string_view("view"),
        std::string_view("submit"),
        std::string_view("update-cookie"),
        std::string_view("cookie-status"),
        std::string_view("version")
    };

    inline bool is_valid(std::string_view cmd) {
        return std::find(commands.begin(), commands.end(), cmd) != commands.end();
    }
}

// Terminal formatting
namespace term {
    const std::string reset = "\033[0m";
    const std::string bold = "\033[1m";
    const std::string dim = "\033[2m";
    const std::string italic = "\033[3m";
    const std::string underline = "\033[4m";
    const std::string yellow = "\033[33m";
    const std::string cyan = "\033[36m";
    const std::string white = "\033[37m";
    const std::string red = "\033[31m";
    const std::string green = "\033[32m";
}

// Core data structures
struct Config {
    fs::path cache_dir;
    fs::path cookie_file;
    fs::path cookie_timestamp_file;
    fs::path inputs_dir;
    fs::path problems_dir;
    fs::path answers_dir;
};

struct WriteBuffer {
    char* data;
    size_t size;
    size_t capacity;
};

enum class SubmitResult {
    CORRECT,
    INCORRECT,
    TOO_HIGH,
    TOO_LOW,
    RATE_LIMITED,
    ERROR
};

struct SubmitResponse {
    SubmitResult result;
    std::string message;
};

// Cache management
class Cache {
private:
    struct CacheEntry {
        std::string data;
        time_t timestamp;
    };

    static constexpr size_t MAX_ENTRIES = 25;
    std::unordered_map<std::string, CacheEntry> entries;
    std::mutex cacheMutex;

public:
    void put(const std::string& key, const std::string& data);
    std::optional<std::string> get(const std::string& key);
};

// Memory-mapped file handling
class MappedFile {
private:
    void* mapped_data;
    size_t file_size;
    int fd;

public:
    explicit MappedFile(const fs::path& path);
    std::string_view getData() const;
    ~MappedFile();
};

// Memory management
class MemoryPool {
private:
    static constexpr size_t BLOCK_SIZE = 4096;
    static constexpr size_t MAX_ALLOC = 256;

    struct Block {
        char data[BLOCK_SIZE];
        size_t used = 0;
        Block* next = nullptr;
    };

    Block* current_block;
    std::vector<Block*> blocks;

public:
    MemoryPool();
    void* allocate(size_t size);
    ~MemoryPool();
};

// Batch operations
class BatchFetcher {
public:
    static void fetchMultiple(const Config& config, int year,
                            const std::vector<int>& days);
};

// Core functionality declarations
Config initialize_config();
size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);
std::string get_cookie(const Config& config);
std::string get_cached_input(const Config& config, int year, int day);
void cache_input(const Config& config, int year, int day, const std::string& input);
void update_cookie(const Config& config);
bool is_cookie_valid(const Config& config);
std::string viewProblem(int year, int day, const std::string& cookie);
std::string get_cached_problem(const Config& config, int year, int day);
void cache_problem(const Config& config, int year, int day, const std::string& problem);
void getCurrentYearAndDay(int &year, int &day);
bool isProblemAvailable(int year, int day);
std::string fetchAdventOfCodeInput(int year, int day, const std::string &cookie);
std::string formatText(const std::string& text, size_t width = 80);
std::string extractText(GumboNode* node);
SubmitResponse submitAnswer(int year, int day, int part,
                          const std::string& answer, const std::string& cookie);

// Thread-local memory pool
static thread_local MemoryPool htmlPool;

#endif // AOCLI_HH
