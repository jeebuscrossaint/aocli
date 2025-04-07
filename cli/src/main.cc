#include "aocli.hh"
#include <iostream>
#include <string>
#include <string_view>

namespace {
    void print_help() {
        std::cout <<
            "Usage: aocli <command> [options] [arguments]\n\n"

            "Commands:\n"
            "  fetch         Fetch puzzle input\n"
            "                aocli fetch [day] [year]\n"
            "                aocli fetch -f [day] [year]      (force refresh)\n\n"

            "  view          View puzzle description\n"
            "                aocli view [day] [year]\n"
            "                aocli view -f [day] [year]       (force refresh)\n\n"

            "  submit        Submit puzzle answer\n"
            "                aocli submit <part> <answer> [day] [year]\n"
            "                part: 1 or 2\n"
            "                answer: your solution\n\n"

            "  update-cookie Update session cookie\n"
            "                aocli update-cookie\n\n"

            "  cookie-status Check cookie validity\n"
            "                aocli cookie-status\n\n"

            "Options:\n"
            "  -f, --refresh Force refresh cached content\n\n"

            "Arguments:\n"
            "  day           Puzzle day (1-25)\n"
            "  year          Puzzle year (2015-present)\n"
            "                If not provided, defaults to current day/year\n"
            "                during December, or day 1 otherwise\n\n"

            "Examples:\n"
            "  aocli fetch                    Fetch today's input\n"
            "  aocli fetch 1 2023             Fetch day 1, 2023 input\n"
            "  aocli view -f 5 2022           View day 5, 2022 puzzle (force refresh)\n"
            "  aocli submit 1 \"123\" 3 2023    Submit 123 as part 1 answer for day 3, 2023\n"
            "  aocli update-cookie            Update session cookie\n";
    }

    void handle_submit_response(const SubmitResponse& response) {
        std::cout << std::string(80, '=') << '\n';

        switch (response.result) {
            case SubmitResult::CORRECT:
                std::cout << term::bold << term::green
                         << "✓ Correct Answer!" << term::reset << '\n';
                break;
            case SubmitResult::TOO_HIGH:
                std::cout << term::bold << term::red
                         << "⚠ Too High!" << term::reset << '\n';
                break;
            case SubmitResult::TOO_LOW:
                std::cout << term::bold << term::red
                         << "⚠ Too Low!" << term::reset << '\n';
                break;
            case SubmitResult::RATE_LIMITED:
                std::cout << term::bold << term::yellow
                         << "⚠ Rate Limited!" << term::reset << '\n';
                break;
            case SubmitResult::INCORRECT:
                std::cout << term::bold << term::red
                         << "✗ Incorrect Answer!" << term::reset << '\n';
                break;
            case SubmitResult::ERROR:
                std::cout << term::bold << term::red
                         << "⚠ Error!" << term::reset << '\n';
                break;
        }

        std::cout << std::string(80, '-') << '\n'
                 << formatText(response.message) << '\n'
                 << std::string(80, '=') << '\n';
    }

    void display_problem(const std::string& problem, int day) {
        // Format and display the header
        std::cout << term::bold << term::yellow
                 << std::string(80, '=') << "\n--- Day " << day << ": ";

        // Extract and display title
        size_t titleStart = problem.find(": ") + 2;
        size_t titleEnd = problem.find('\n', titleStart);
        if (titleStart != std::string::npos && titleEnd != std::string::npos) {
            std::cout << problem.substr(titleStart, titleEnd - titleStart);
        }
        std::cout << " ---\n" << std::string(80, '=')
                 << term::reset << "\n\n";

        // Split and format parts
        size_t part2Start = problem.find("--- Part Two ---");
        if (part2Start != std::string::npos) {
            // Print Part 1
            std::cout << term::bold << term::cyan << "Part One:"
                     << term::reset << '\n'
                     << std::string(40, '-') << '\n'
                     << formatText(problem.substr(0, part2Start)) << '\n'
                     << term::bold << term::cyan << "Part Two:"
                     << term::reset << '\n'
                     << std::string(40, '-') << '\n'
                     << formatText(problem.substr(part2Start + 14)) << '\n';
        } else {
            // Only Part 1 available
            std::cout << term::bold << term::cyan << "Part One:"
                     << term::reset << '\n'
                     << std::string(40, '-') << '\n'
                     << formatText(problem) << '\n';
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    Config config = initialize_config();
    std::string cookie = get_cookie(config);
    std::string_view command(argv[1]);

    // Process command line arguments
    bool forceRefresh = false;
    std::vector<std::string_view> args;
    args.reserve(argc - 1);

    for (int i = 1; i < argc; i++) {
        std::string_view arg(argv[i]);
        if (arg == "-f" || arg == "--refresh") {
            forceRefresh = true;
        } else {
            args.push_back(arg);
        }
    }

    try {
        if (!cmd::is_valid(command)) {
            std::cerr << "Unknown command: " << command << std::endl;
            print_help();
            return 1;
        }

        // Handle submit command
        if (command == "submit") {
            if (args.size() < 3) {
                std::cerr << "Usage: aocli submit <part> <answer> [day] [year]"
                         << std::endl;
                return 1;
            }

            int part = std::stoi(std::string(args[1]));
            std::string answer(args[2]);

            // Get default year and day
            int day = 0, year = 0;
            getCurrentYearAndDay(year, day);

            // Override with provided day/year if any
            if (args.size() > 3) day = std::stoi(std::string(args[3]));
            if (args.size() > 4) year = std::stoi(std::string(args[4]));

            if (part != 1 && part != 2) {
                throw std::runtime_error("Part must be 1 or 2");
            }

            if (!isProblemAvailable(year, day)) {
                throw std::runtime_error(
                    "Problem not available yet (Year: " +
                    std::to_string(year) + ", Day: " +
                    std::to_string(day) + ")"
                );
            }

            std::cout << term::bold << "Submitting answer for Year " << year
                     << " Day " << day << " Part " << part << "..."
                     << term::reset << std::endl;

            SubmitResponse response = submitAnswer(year, day, part,
                                                answer, cookie);
            handle_submit_response(response);
            return 0;
        }

        // Handle other commands
        int day = 0, year = 0;
        getCurrentYearAndDay(year, day);

        // Override defaults if arguments provided
        if (args.size() > 1) day = std::stoi(std::string(args[1]));
        if (args.size() > 2) year = std::stoi(std::string(args[2]));

        // Validate the date for commands that need it
        if (command == "fetch" || command == "view") {
            if (!isProblemAvailable(year, day)) {
                throw std::runtime_error(
                    "Problem not available yet (Year: " +
                    std::to_string(year) + ", Day: " +
                    std::to_string(day) + ")"
                );
            }
        }

        if (command == "fetch") {
            std::string input = get_cached_input(config, year, day);
            if (forceRefresh || input.empty()) {
                input = fetchAdventOfCodeInput(year, day, cookie);
                cache_input(config, year, day, input);
            }
            std::cout << input;
        }
        else if (command == "view") {
            std::string problem;
            if (!forceRefresh) {
                problem = get_cached_problem(config, year, day);
            }

            // Check if we need to refresh for Part 2
            if (!problem.empty() &&
                problem.find("--- Part Two ---") == std::string::npos) {
                std::string fresh_problem = viewProblem(year, day, cookie);
                if (fresh_problem.find("--- Part Two ---") != std::string::npos) {
                    problem = fresh_problem;
                    cache_problem(config, year, day, problem);
                }
            }

            if (problem.empty()) {
                problem = viewProblem(year, day, cookie);
                cache_problem(config, year, day, problem);
            }

            display_problem(problem, day);
        }
        else if (command == "update-cookie") {
            update_cookie(config);
            std::cout << "Cookie updated successfully.\n";
        }
        else if (command == "cookie-status") {
            std::cout << (is_cookie_valid(config)
                         ? "Cookie is valid.\n"
                         : "Cookie is invalid or expired.\n");
        }
        else if (command == "version") {
            std::cout << "aocli v1.0 using libcurl and gumbo-parser\n";
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
