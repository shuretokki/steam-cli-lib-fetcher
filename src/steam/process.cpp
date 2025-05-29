#include "steam/data.hpp"

#include "steam/handler.hpp" // For calling handler functions
#include "steam/process.hpp" // Includes numeric, sstream, data.hpp, base.hpp

// fmt/core.h, fmt/color.h are included via base.hpp -> data.hpp -> process.hpp

#include <algorithm> // For std::min
#include <iostream>  // For std::stoi error messages (though fmt handles printing)
#include <stdexcept> // For std::runtime_error
#include <string>
#include <vector>

using namespace fmt;

STEAM_BEGIN_NAMESPACE
namespace process {

std::vector<std::string> ParseCommandLine(const std::string& command_line)
{
        std::vector<std::string> arguments;
        std::string              current_argument;
        std::stringstream        ss(command_line);
        char                     ch;
        bool                     in_quotes = false;

        while (ss.get(ch)) {
                if (ch == '"') {
                        in_quotes = !in_quotes;
                        if (!in_quotes && !current_argument.empty()) {
                        } else if (in_quotes && !current_argument.empty()) {
                                arguments.push_back(current_argument);
                                current_argument.clear();
                        }
                } else if (std::isspace(ch) && !in_quotes) {
                        if (!current_argument.empty()) {
                                arguments.push_back(current_argument);
                                current_argument.clear();
                        }
                } else {
                        current_argument += ch;
                }
        }
        if (!current_argument.empty()) {
                arguments.push_back(current_argument);
        }
        return arguments;
}

void ProcessUserCommand(const std::vector<std::string>& arguments)
{
        if (arguments.empty()) {
                return;
        }
        const std::string& command = arguments[0];

        if (command == "fetch") {
                if (arguments.size() < 2) {
                        print(fg(color::indian_red), "Error: 'fetch' requires a SteamID or Vanity URL.\n");
                        print(fg(color::yellow), "Usage: fetch <SteamID64/VanityURLName>\n");
                } else {
                        handler::FetchGamesFromSteamApi(arguments[1]);
                }
        } else if (command == "search") {
                if (arguments.size() < 2) {
                        print(fg(color::indian_red), "Error: 'search' requires a game name prefix.\n");
                        print(fg(color::yellow), "Usage: search <prefix>\n");
                } else {
                        handler::HandleSearchCommand(arguments[1]);
                }
        } else if (command == "count") {
                handler::HandleCountPlayedCommand();
        } else if (command == "list") {
                char list_format = ' ';
                if (arguments.size() > 1) {
                        if (arguments[1] == "-l")
                                list_format = 'l';
                        else if (arguments[1] == "-n")
                                list_format = 'n';
                        else if (arguments[1] == "-p")
                                list_format = 'p';
                        else {
                                print(fg(color::indian_red), "Error: Unknown option '{}' for list.\n", arguments[1]);
                                print(fg(color::yellow), "Usage: list [-l | -n | -p]\n");
                                return;
                        }
                }
                handler::HandleListGamesCommand(list_format);
        } else if (command == "help") {
                handler::ShowHelp();
        } else if (command == "export") {
                if (arguments.size() < 2) {
                        print(fg(color::indian_red), "Error: 'export' requires a filename.\n");
                        print(fg(color::yellow), "Usage: export <filename_base>\n");
                } else {
                        handler::HandleExportToCsvCommand(arguments[1]);
                }
        } else if (command == "exit") {
                throw std::runtime_error("exit");
        } else if (command == "history") {
                HandleHistoryCommand(arguments);
        } else {
                print(fg(color::indian_red), "Error: Unknown command '{}'. Type 'help' for commands.\n", command);
        }
}

void AddCommandToHistory(const std::string& command_line)
{
        if (command_line.empty()) {
                return;
        }
        if (kMaxCommandHistorySize > 0 && steam_command_history.size() >= kMaxCommandHistorySize) {
                steam_command_history.pop_front();
        }
        if (kMaxCommandHistorySize <= 0 || steam_command_history.size() < kMaxCommandHistorySize
            || steam_command_history.empty()) {
                steam_command_history.push_back(command_line);
        }
}

void HandleHistoryCommand(const std::vector<std::string>& arguments)
{
        int count = kDefaultHistoryDisplayCount;
        if (arguments.size() > 1) {
                try {
                        count = std::stoi(arguments[1]);
                        if (count <= 0) {
                                print(
                                    fg(color::yellow),
                                    "History count must be positive. Showing default ({}).\n",
                                    kDefaultHistoryDisplayCount);
                                count = kDefaultHistoryDisplayCount;
                        }
                } catch (const std::invalid_argument&) {
                        print(
                            fg(color::indian_red),
                            "Invalid number for history count: '{}'. Showing default ({}).\n",
                            arguments[1],
                            kDefaultHistoryDisplayCount);
                        count = kDefaultHistoryDisplayCount;
                } catch (const std::out_of_range&) {
                        print(
                            fg(color::indian_red),
                            "Number for history count too large: '{}'. Showing default ({}).\n",
                            arguments[1],
                            kDefaultHistoryDisplayCount);
                        count = kDefaultHistoryDisplayCount;
                }
        }

        if (steam_command_history.empty()) {
                print(fg(color::yellow), "Command history is empty.\n");
                return;
        }

        print(fg(color::cyan) | emphasis::bold, "-- Command History (Last up to {} entries) --\n", count);
        int num_to_show     = std::min(static_cast<int>(steam_command_history.size()), count);

        int displayed_count = 0;
        for (auto it = steam_command_history.rbegin();
             it != steam_command_history.rend() && displayed_count < num_to_show;
             ++it, ++displayed_count) {
                print(fg(color::white), "- {}\n", *it);
        }
        print(fg(color::cyan), "---------------------------------------\n");
}

} // namespace process
STEAM_END_NAMESPACE