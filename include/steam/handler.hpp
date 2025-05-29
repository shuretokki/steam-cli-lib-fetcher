#ifndef STEAM_HANDLER_HPP
#define STEAM_HANDLER_HPP

#include "data.hpp"
#include "loader.hpp"
#include "prefix.hpp"
#include "process.hpp"
#include "utility.hpp"

STEAM_BEGIN_NAMESPACE

namespace handler {
/**
 * @brief Fetches game data from the Steam API for a given Steam ID or vanity
 * URL.
 * @param steam_id_or_vanity_url The user's 17-digit SteamID64 or their custom
 * vanity URL name.
 * @return True if data fetching was successful, false otherwise.
 * ? Should this function differentiate between network errors and private
 * profiles?
 */
bool FetchGamesFromSteamApi(const std::string& steam_id_or_vanity_url);

/**
 * @brief Searches for games using the prefix tree and prints the results.
 * @param name_prefix The prefix of the game name to search for.
 */
void HandleSearchCommand(const std::string& name_prefix);

/**
 * @brief Counts and prints the number of games that have been played (playtime >
 * 0).
 */
void HandleCountPlayedCommand();

/**
 * @brief Exports the current game list to a CSV file.
 * @param output_filename The base name for the output CSV file (e.g.,
 * "my_games").
 */
void HandleExportToCsvCommand(const std::string& output_filename);

/**
 * @brief Lists games in various formats.
 * @param list_format A character indicating the format:
 * ' ' (default): Simple list of names.
 * 'l': Detailed table (AppID, Name, Playtime).
 * 'n': Grouped by first letter (Name, AppID).
 * 'p': Sorted by playtime (AppID, Name, Playtime).
 */
void HandleListGamesCommand(char list_format = ' ');

/**
 * @brief Displays help information, including available commands and current
 * user data if fetched.
 */
void ShowHelp();
} // namespace handler

STEAM_END_NAMESPACE
#endif