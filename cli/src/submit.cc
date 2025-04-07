#include "aocli.hh"
#include <functional>

namespace {
    // Extract response message from HTML response
    std::string extractResponseMessage(const std::string& html) {
        GumboOutput* output = gumbo_parse(html.c_str());
        std::string message;

        // Function to recursively search for main article content
        std::function<void(GumboNode*)> findMainArticle;
        findMainArticle = [&message, &findMainArticle](GumboNode* node) {
            if (node->type != GUMBO_NODE_ELEMENT) {
                return;
            }

            // Look for <article> or <main> tags
            if (node->v.element.tag == GUMBO_TAG_ARTICLE ||
                node->v.element.tag == GUMBO_TAG_MAIN) {
                message = extractText(node);
                return;
            }

            // Recursively search children
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                findMainArticle(static_cast<GumboNode*>(children->data[i]));
                if (!message.empty()) {
                    break;
                }
            }
        };

        findMainArticle(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        // Provide fallback message if parsing failed
        if (message.empty()) {
            message = "Unable to parse server response. "
                     "Please check the answer on the website.";
        }

        return message;
    }

    // Parse server response and determine result type
    SubmitResponse parseResponse(const std::string& html) {
        std::string message = extractResponseMessage(html);
        SubmitResponse response;
        response.message = message;

        // Determine response type based on message content
        if (message.find("That's the right answer") != std::string::npos) {
            response.result = SubmitResult::CORRECT;
        }
        else if (message.find("too high") != std::string::npos) {
            response.result = SubmitResult::TOO_HIGH;
        }
        else if (message.find("too low") != std::string::npos) {
            response.result = SubmitResult::TOO_LOW;
        }
        else if (message.find("wait") != std::string::npos) {
            response.result = SubmitResult::RATE_LIMITED;
        }
        else if (message.find("not the right answer") != std::string::npos) {
            response.result = SubmitResult::INCORRECT;
        }
        else {
            response.result = SubmitResult::ERROR;
        }

        return response;
    }
}

SubmitResponse submitAnswer(int year, int day, int part,
                          const std::string& answer,
                          const std::string& cookie) {
    // Initialize CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    // Prepare request URL and data
    const std::string url = "https://adventofcode.com/" +
                           std::to_string(year) + "/day/" +
                           std::to_string(day) + "/answer";

    std::string postData;
    postData.reserve(64);  // Pre-allocate space for efficiency
    postData = "level=" + std::to_string(part) + "&answer=" + answer;

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
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        const std::string cookieStr = "session=" + cookie;
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookieStr.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
                        "github.com/your-username/aocli v1.0");

        // Perform request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error(
                "Failed to submit answer: " +
                std::string(curl_easy_strerror(res))
            );
        }

        // Convert buffer to string and parse response
        std::string response(buffer.data, buffer.size);
        SubmitResponse result = parseResponse(response);

        // Handle empty response
        if (result.message.empty()) {
            result.result = SubmitResult::ERROR;
            result.message = "Failed to parse response from server";
        }

        // Cleanup and return
        free(buffer.data);
        curl_easy_cleanup(curl);
        return result;
    }
    catch (...) {
        free(buffer.data);
        curl_easy_cleanup(curl);
        throw;
    }
}
