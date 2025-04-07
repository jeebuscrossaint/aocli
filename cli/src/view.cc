#include "aocli.hh"
#include <gumbo.h>
#include <algorithm>

std::string extractText(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    }

    if (node->type == GUMBO_NODE_ELEMENT) {
        // Pre-allocate space based on number of children
        std::string text;
        GumboVector* children = &node->v.element.children;
        text.reserve(children->length * 64);  // Estimate average 64 chars per child

        // Process all child nodes
        for (unsigned int i = 0; i < children->length; ++i) {
            std::string child_text = extractText(
                static_cast<GumboNode*>(children->data[i])
            );

            if (!child_text.empty()) {
                // Add space between text blocks if needed
                if (!text.empty() &&
                    text.back() != '\n' &&
                    child_text.front() != '\n') {
                    text += ' ';
                }
                text += child_text;
            }
        }

        // Add appropriate formatting based on HTML element type
        switch (node->v.element.tag) {
            case GUMBO_TAG_P:
                if (!text.empty()) {
                    text.insert(0, 1, '\n');
                    text.push_back('\n');
                }
                break;

            case GUMBO_TAG_BR:
                text = "\n";
                break;

            case GUMBO_TAG_H1:
            case GUMBO_TAG_H2:
                if (!text.empty()) {
                    text.insert(0, 1, '\n');
                    text += "\n\n";
                }
                break;

            case GUMBO_TAG_PRE:
                if (!text.empty()) {
                    text.insert(0, 1, '\n');
                    text.push_back('\n');
                }
                break;

            default:
                // Other HTML tags don't need special formatting
                break;
        }

        return text;
    }

    return {};
}

std::string formatText(const std::string& text, size_t width) {
    std::string result;
    result.reserve(text.length());
    std::string current_line;
    current_line.reserve(width);

    std::istringstream iss(text);
    std::string word;

    while (iss >> word) {
        if (current_line.empty()) {
            current_line = word;
        }
        else if (current_line.length() + word.length() + 1 <= width) {
            current_line += ' ' + word;
        }
        else {
            result += current_line + '\n';
            current_line = word;
        }
    }

    if (!current_line.empty()) {
        result += current_line + '\n';
    }

    return result;
}

std::string get_cached_problem(const Config& config, int year, int day) {
    fs::path problem_file = config.problems_dir /
                           (std::to_string(year) + "_" +
                            std::to_string(day) + ".txt");

    if (fs::exists(problem_file)) {
        std::ifstream file(problem_file, std::ios::binary | std::ios::ate);
        if (!file) {
            return "";
        }

        // Get file size and reserve string capacity
        auto size = file.tellg();
        if (size == -1) {
            return "";
        }

        std::string content;
        content.reserve(size);
        file.seekg(0);

        // Read entire file at once
        content.assign(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        return content;
    }

    return "";
}

void cache_problem(const Config& config, int year, int day,
                  const std::string& problem) {
    fs::path problem_file = config.problems_dir /
                           (std::to_string(year) + "_" +
                            std::to_string(day) + ".txt");

    std::ofstream file(problem_file, std::ios::binary);
    if (!file) {
        return;
    }

    // Set up buffering for better performance
    static constexpr size_t BUFFER_SIZE = 8192;  // 8KB buffer
    char buffer[BUFFER_SIZE];
    file.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

    // Write entire content at once
    file.write(problem.data(), problem.size());
}

std::string findProblemDescription(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return "";
    }

    // Look for main article tags
    if (node->v.element.tag == GUMBO_TAG_ARTICLE ||
        node->v.element.tag == GUMBO_TAG_MAIN) {
        std::string text = extractText(node);

        // Make sure we got meaningful content
        if (text.find("Day") != std::string::npos) {
            return text;
        }
    }

    // Recursively search children
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        std::string desc = findProblemDescription(
            static_cast<GumboNode*>(children->data[i])
        );
        if (!desc.empty()) {
            return desc;
        }
    }

    return "";
}

std::string viewProblem(int year, int day, const std::string& cookie) {
    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    // Prepare request URL
    std::string url = "https://adventofcode.com/" +
                     std::to_string(year) + "/day/" +
                     std::to_string(day);

    // Initialize response buffer
    WriteBuffer buffer = {
        .data = static_cast<char*>(malloc(4096)),  // 4KB initial buffer
        .size = 0,
        .capacity = 4096
    };

    if (!buffer.data) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to allocate memory for response buffer");
    }

    try {
        // Setup CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        std::string cookieStr = "session=" + cookie;
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookieStr.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
                        "github.com/your-username/aocli v1.0");

        // Perform request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error(
                "Failed to fetch problem page: " +
                std::string(curl_easy_strerror(res))
            );
        }

        // Convert buffer to string
        std::string html(buffer.data, buffer.size);

        // Cleanup curl and buffer
        free(buffer.data);
        curl_easy_cleanup(curl);

        // Parse HTML
        GumboOutput* output = gumbo_parse(html.c_str());
        std::string problemText = findProblemDescription(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        if (problemText.empty()) {
            throw std::runtime_error("Failed to parse problem description");
        }

        return problemText;
    }
    catch (...) {
        free(buffer.data);
        curl_easy_cleanup(curl);
        throw;
    }
}
