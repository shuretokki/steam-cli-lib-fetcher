#ifndef STEAM_PROCESS_HPP
#define STEAM_PROCESS_HPP

#include <numeric> /* For std::iota */
#include <sstream> /* For std::stringstream in ParseCommandLine */
#include "data.hpp"

STEAM_BEGIN_NAMESPACE

namespace process {
/**
 * @brief Parses a command line string into a vector of arguments.
 * * Handles arguments enclosed in double quotes.
 * @param command_line The raw command string entered by the user.
 * @return A vector of strings, where each string is an argument.
 */
std::vector<std::string> ParseCommandLine(const std::string& command_line);

/**
 * @brief Processes the parsed command arguments and executes the corresponding
 * action.
 * @param arguments A vector of command arguments.
 * @throw std::runtime_error with message "exit" to signal program termination.
 */
void ProcessUserCommand(const std::vector<std::string>& arguments);

/**
 * @brief Adds a command line to the command history.
 * @param command_line The command line string to add.
 */
void AddCommandToHistory(const std::string& command_line);

/**
 * @brief Handles the 'history' command to display recent commands.
 * @param arguments Parsed command arguments.
 */
void HandleHistoryCommand(const std::vector<std::string>& arguments);
} // namespace process
STEAM_END_NAMESPACE
#endif