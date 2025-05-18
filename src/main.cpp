#include <iostream>
#include "slf.hpp"

using namespace fmt;

int main()
{
        if (std::filesystem::exists(data_filename)) {
                std::filesystem::remove(data_filename);
        }

        load_games();

        print(
            fg(color::gold) | emphasis::bold,
            "Steam Game Fetcher v1.0 - Type 'help' for commands\n");
        print(fg(color::gold), "╾━━━━━━━━╼\n");

        std::string line;
        while (true) {
                print(fg(color::light_cyan) | emphasis::bold, "> ");
                std::getline(std::cin, line);
                if (line.empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: Empty input. Type 'help' for commands.\n");
                        continue;
                }

                std::vector<std::string> args = parse_command(line);
                try {
                        process_command(args);
                } catch (const std::exception& e) {
                        if (std::string(e.what()) == "exit") {
                                if (std::filesystem::exists(data_filename)) {
                                        std::filesystem::remove(data_filename);
                                }
                                print(fg(color::light_sea_green), "Goodbye!\n");
                                break;
                        }
                        print(fg(color::indian_red), "Error: {}\n", e.what());
                }
        }

        return 0;
}