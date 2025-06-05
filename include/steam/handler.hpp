#ifndef STEAM_HANDLER_HPP
#define STEAM_HANDLER_HPP

#include "data.hpp"
#include "graph.hpp"
#include "loader.hpp"
#include "prefix.hpp"
#include "process.hpp"
#include "undo.hpp"
#include "utility.hpp"

STEAM_BEGIN_NAMESPACE

namespace handler {
/**
 * @brief Fetches game data from the Steam API for a given Steam ID or vanity
 * URL.
 * @param steam_id_or_vanity_url The user's 17-digit SteamID64 or their custom
 * vanity URL name.
 * @return True if data fetching was successful, false otherwise.
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

/**
 * @brief Resolves a game identifier (name, prefix, or AppID) to an AppID.
 * @param identifier The game identifier string.
 * @param found_game_name Optional output parameter to store the resolved game's name.
 * @return The AppID if found, otherwise 0.
 */
int ResolveGameToAppId(const std::string& identifier, std::string* found_game_name = nullptr);

/**
 * @brief Handles the 'relate' command to create a relationship between two games.
 * @param game1_id_str Identifier for the first game.
 * @param game2_id_str Identifier for the second game.
 */
void HandleRelateCommand(const std::string& game1_id_str, const std::string& game2_id_str);

/**
 * @brief Handles the 'recommendations' (or 'recs') command to show related games.
 * @param game_id_str Identifier for the game to get recommendations for.
 */
void HandleRecommendationsCommand(const std::string& game_id_str);

/**
 * @brief Handles the 'undo' command.
 */
void HandleUndoCommand();

} // namespace handler

STEAM_END_NAMESPACE
#endif