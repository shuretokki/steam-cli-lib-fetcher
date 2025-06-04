#include "steam/steam.hpp"

int main()
{
        using namespace fmt;
        using namespace steam;
        loader::LoadGamesDataFromJson();

        print(fg(color::gold) | emphasis::bold, "v1.1 - Type 'help' for commands");
        print(fg(color::gold), "\n:::::::::::::::::::::::\n");
        std::string user_input_line;

        /****************************************************
         * ? INPUT -> PARSING ARGUMENT -> REPEAT TIL 'EXIT' *
         ****************************************************/
        while (true) {

                /**Input**
                 ****/
                print(fg(color::light_cyan) | emphasis::bold, "> ");
                if (!std::getline(std::cin, user_input_line)) {
                        if (std::cin.eof()) {
                                print(fg(color::yellow), "{}EOF detected. Exiting...{}", '\n');
                        } else {
                                print(fg(color::indian_red), "{}Input error. Exiting...{}", '\n');
                        }
                        break;
                }
                if (user_input_line.empty()) {
                        print(fg(color::yellow), "Tip: Type 'help' for commands.\n");
                        continue;
                }

                /**Argument parsing**
                 ****/
                std::vector<std::string> arguments = process::ParseCommandLine(user_input_line);
                if (arguments.empty()) {
                        continue;
                }

                const std::string& command_name = arguments[0];
                try {
                        process::ProcessUserCommand(arguments);
                        if (command_name != "history" && command_name != "exit") {
                                process::AddCommandToHistory(user_input_line);
                        }
                } catch (const std::runtime_error& e) {

                        if (std::string(e.what()) == "exit") {
                                print(fg(color::light_sea_green), "Exiting application. Goodbye!\n");
                                break;
                        }
                        print(fg(color::indian_red), "Runtime Error: {}\n", e.what());
                } catch (const std::exception& e) {
                        print(fg(color::indian_red), "Standard Exception: {}\n", e.what());
                }
        }

        /**End program**
         ****/
        return 0;
}

/*

*   When to Use std::runtime_error:

!   External Failures:
When operations like file access, network communication, or memory allocation fail.
!   Invalid Input:
When the program receives input that is outside of its expected range or format.
!   Resource Issues:
When the program encounters problems with system resources, such as running out of memory.
!   Unexpected State:
When the program reaches an unexpected state that cannot be resolved by the program's logic.

*/