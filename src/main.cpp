#include <iostream>
#include "slf.hpp"

using namespace fmt;

/** @brief Fungsi main
 *  @return 0 jika sukses
 */
int main()
{
        if (std::filesystem::exists(SLF_filename)) {
                std::filesystem::remove(SLF_filename);
                if (SLF_verbose) {
                        print(
                            fg(color::yellow),
                            "Cleared previous data on startup.\n");
                }
        }

        SLF_load_from_file();

        print(
            fg(color::gold) | emphasis::bold,
            "Steam Library Fetcher v1.0 - Type 'help' for commands\n");
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

                std::vector<std::string> args = SLF_parse_command(line);
                try {
                        SLF_process_command(args);
                } catch (const std::exception& e) {
                        if (std::string(e.what()) == "exit") {
                                if (std::filesystem::exists(SLF_filename)) {
                                        std::filesystem::remove(SLF_filename);
                                        if (SLF_verbose) {
                                                print(
                                                    fg(color::yellow),
                                                    "Removed {} on exit.\n",
                                                    SLF_filename);
                                        }
                                }
                                print(fg(color::light_sea_green), "Goodbye!\n");
                                break;
                        }
                        print(fg(color::indian_red), "Error: {}\n", e.what());
                }
        }

        return 0;
}