// src/steam/process.cpp
#include "steam/process.hpp"
#include "steam/data.hpp"
#include "steam/handler.hpp"

#include <algorithm> // For std::min in HandleHistoryCommand
#include <iostream>
#include <sstream>   // For std::stringstream in ParseCommandLine
#include <stdexcept> // For std::runtime_error, std::invalid_argument, std::out_of_range
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
                        // If exiting quotes and current_argument is not empty, it's part of the quoted arg.
                        // If entering quotes and current_argument is not empty, it's a separate arg before the quote.
                        if (!in_quotes) {                        // Just finished a quote
                                if (!current_argument.empty()) { // Add the quoted argument
                                        arguments.push_back(current_argument);
                                        current_argument.clear();
                                }
                        } else {                                 // Just started a quote
                                if (!current_argument.empty()) { // Add argument before the quote
                                        arguments.push_back(current_argument);
                                        current_argument.clear();
                                }
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
                        // Collect all arguments after "search" for multi-word search terms if not quoted
                        // For now, assume ParseCommandLine handles quoted arguments correctly,
                        // and unquoted multi-word search will take arguments[1] only.
                        // If search "game name", arguments will be ["search", "game name"]
                        // If search game name, arguments will be ["search", "game", "name"]
                        // We'll pass arguments[1] to handler, which might need to be smarter or
                        // users must quote multi-word prefixes. For now, pass arguments[1].
                        std::string search_term = arguments[1];
                        if (arguments.size() > 2) { // if unquoted multi-word, join them. Not ideal.
                                for (size_t i = 2; i < arguments.size(); ++i)
                                        search_term += " " + arguments[i];
                                print(
                                    fg(color::yellow),
                                    "Searching for \"{}\". For multi-word search, consider quotes: search \"{}\"\n",
                                    search_term,
                                    search_term);
                        }
                        handler::HandleSearchCommand(search_term);
                }
        } else if (command == "count") {
                handler::HandleCountPlayedCommand();
        } else if (command == "list") {
                char list_format = ' '; // Default format
                if (arguments.size() > 1) {
                        if (arguments[1] == "-l")
                                list_format = 'l';
                        else if (arguments[1] == "-n")
                                list_format = 'n';
                        else if (arguments[1] == "-p")
                                list_format = 'p';
                        else {
                                print(fg(color::indian_red), "Error: Unknown option '{}' for list.\n", arguments[1]);
                                print(fg(color::yellow), "Usage: list [ -l | -n | -p ]\n");
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
        } else if (command == "relate") {
                if (arguments.size() < 3) {
                        print(
                            fg(color::indian_red),
                            "Error: 'relate' requires two game identifiers (name or AppID).\n");
                        print(fg(color::yellow), "Usage: relate <game1_id_or_name> <game2_id_or_name>\n");
                } else {
                        // Assuming arguments[1] and arguments[2] are the game identifiers.
                        // ParseCommandLine should handle quotes.
                        // For example: relate "game one" "another game" -> args: ["relate", "game one", "another game"]
                        // For example: relate game1 12345 -> args: ["relate", "game1", "12345"]
                        handler::HandleRelateCommand(arguments[1], arguments[2]);
                }
        } else if (command == "recommendations" || command == "recs") {
                if (arguments.size() < 2) {
                        print(fg(color::indian_red), "Error: 'recommendations' requires a game identifier.\n");
                        print(fg(color::yellow), "Usage: recommendations <game_id_or_name>\n");
                } else {
                        // For example: recommendations "my fav game" -> args: ["recommendations", "my fav game"]
                        handler::HandleRecommendationsCommand(arguments[1]);
                }
        } else if (command == "undo") {
                handler::HandleUndoCommand();
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
        // Prevent excessively large history if kMaxCommandHistorySize is 0 or very large
        if (kMaxCommandHistorySize > 0 && steam_command_history.size() >= kMaxCommandHistorySize) {
                steam_command_history.pop_front();
        }
        // Add if not full or if kMaxCommandHistorySize is unlimited (<=0 implies unlimited for this check)
        if (kMaxCommandHistorySize <= 0 || steam_command_history.size() < kMaxCommandHistorySize) {
                if (steam_command_history.empty()
                    || steam_command_history.back() != command_line) { // Avoid duplicate consecutive commands
                        steam_command_history.push_back(command_line);
                }
        }
}

void HandleHistoryCommand(const std::vector<std::string>& arguments)
{
        int count = kDefaultHistoryDisplayCount; // Default from data.hpp
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
        // Iterate from newest to oldest
        for (auto it = steam_command_history.rbegin();
             it != steam_command_history.rend() && displayed_count < num_to_show;
             ++it, ++displayed_count) {
                print(fg(color::white), "{:>3}: {}\n", steam_command_history.size() - displayed_count, *it);
        }
        print(fg(color::cyan), "---------------------------------------\n");
}

} // namespace process
STEAM_END_NAMESPACE