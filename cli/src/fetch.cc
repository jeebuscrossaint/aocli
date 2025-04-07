#include <iostream>
#include <string>
#include <ctime>
#include <cstring>
#include <curl/curl.h>
#include "aocli.hh"

// Callback function for libcurl to write the response into a buffer
size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t realsize = size * nmemb;

    // Handle both string and WriteBuffer types
    if (auto* buffer = static_cast<WriteBuffer*>(userp)) {
        if (buffer->size + realsize > buffer->capacity) {
            const size_t new_capacity = (buffer->capacity + realsize) * 2;
            char* ptr = static_cast<char*>(realloc(buffer->data, new_capacity));
            if (!ptr) return 0;

            buffer->data = ptr;
            buffer->capacity = new_capacity;
        }

        memcpy(&(buffer->data[buffer->size]), contents, realsize);
        buffer->size += realsize;
    }
    else if (auto* str = static_cast<std::string*>(userp)) {
        str->append(static_cast<char*>(contents), realsize);
    }

    return realsize;
}

// Helper function to validate puzzle date
static bool isValidPuzzleDate(int year, int day) {
    return day >= 1 && day <= 25 && year >= 2015;
}

std::string fetchAdventOfCodeInput(int year, int day, const std::string& cookie) {
    // Handle default values
    if (year == 0 || day == 0) {
        getCurrentYearAndDay(year, day);
    }

    // Validate input
    if (!isValidPuzzleDate(year, day)) {
        throw std::runtime_error(
            "Invalid puzzle date:\n"
            "- Year must be 2015 or later\n"
            "- Day must be between 1-25\n"
            "Current request: Year " + std::to_string(year) +
            ", Day " + std::to_string(day)
        );
    }

    // Prepare URL
    const std::string url = "https://adventofcode.com/" +
                           std::to_string(year) + "/day/" +
                           std::to_string(day) + "/input";

    // Initialize response buffer
    WriteBuffer buffer = {
        .data = static_cast<char*>(malloc(4096)),  // 4KB initial buffer
        .size = 0,
        .capacity = 4096
    };

    if (!buffer.data) {
        throw std::runtime_error("Failed to allocate memory for response buffer");
    }

    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        free(buffer.data);
        throw std::runtime_error("Failed to initialize curl");
    }

    try {
        // Setup CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        const std::string cookieStr = "session=" + cookie;
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookieStr.c_str());

        // Perform request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error(
                "Failed to fetch input: " +
                std::string(curl_easy_strerror(res))
            );
        }

        // Convert buffer to string
        std::string response(buffer.data, buffer.size);

        // Cleanup
        free(buffer.data);
        curl_easy_cleanup(curl);

        return response;
    }
    catch (...) {
        free(buffer.data);
        curl_easy_cleanup(curl);
        throw;
    }
}
