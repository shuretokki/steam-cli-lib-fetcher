#include "steam/steam.hpp" /* Steam Library Fetcher */

INCLUDE_

/*
 * * Main entry point for the Steam Game Fetcher application.
 * * Initializes the application, loads data, and enters the command loop.
 */
int main()
{
        using namespace fmt;
        using namespace steam;
        /* Load any existing game data and API key on startup. */
        loader::LoadGamesDataFromJson();

        print(fg(color::gold) | emphasis::bold, "Steam Game Fetcher v1.1 - Type 'help' for commands\n");
        print(fg(color::gold), ":::::::::::::::::::::::\n");

        std::string user_input_line;
        while (true) {
                print(fg(color::light_cyan) | emphasis::bold, "> ");
                if (!std::getline(std::cin, user_input_line)) {
                        /* Handle EOF or input error */
                        if (std::cin.eof()) {
                                print(fg(color::yellow), "\nEOF detected. Exiting...\n");
                        } else {
                                print(fg(color::indian_red), "\nInput error. Exiting...\n");
                        }
                        break;
                }

                if (user_input_line.empty()) {
                        /* Allow empty input to just show a new prompt, or give help */
                        /* print(fg(color::yellow), "Tip: Type 'help' for commands.\n"); */
                        continue;
                }

                std::vector<std::string> arguments = process::ParseCommandLine(user_input_line);

                if (arguments.empty()) {
                        continue;
                }

                const std::string& command_name = arguments[0];

                try {
                        process::ProcessUserCommand(arguments);
                        // If ProcessUserCommand did not throw, and it wasn't an exit command, add to history.
                        // The "exit" case is handled by the catch block.
                        if (command_name != "history" && command_name != "exit") {
                                process::AddCommandToHistory(user_input_line);
                        }
                } catch (const std::runtime_error& e) {
                        if (std::string(e.what()) == "exit") {
                                print(fg(color::light_sea_green), "Exiting application. Goodbye!\n");
                                break; /* Exit the while loop */
                        }
                        /* For other runtime errors from ProcessUserCommand (if any) */
                        print(fg(color::indian_red), "Runtime Error: {}\n", e.what());
                } catch (const std::exception& e) {
                        /* Catch any other standard exceptions */
                        print(fg(color::indian_red), "Standard Exception: {}\n", e.what());
                }
        }

        /*
         * ! Optional: Decide if data should be saved on exit.
         * * Currently, data is saved after a successful 'fetch' or 'load'.
         * * If you want to save any changes made through other commands (if any were to modify data),
         * * you could call steam::SaveGamesDataToJson(); here.
         * * However, for this application, saving on fetch/load is sufficient.
         */

        /*
         * ? Should we clean up the games.json file on exit?
         * * The original code removed it if it existed at the start, which seems
         * * counter-intuitive for persistent data. This version loads from it.
         * * Let's not remove it on exit to preserve fetched data.
         */

        return 0;
}
