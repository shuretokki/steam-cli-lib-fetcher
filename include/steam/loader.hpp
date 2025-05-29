#ifndef STEAM_LOADER_HPP
#define STEAM_LOADER_HPP
#include "api_key.hpp"
#include "data.hpp"

STEAM_BEGIN_NAMESPACE

namespace loader {

/**
 * @brief Saves the currently fetched game and user data to a JSON file.
 * * The data is saved to a file specified by GetGamesDataPath().
 */
void SaveGamesDataToJson();

/**
 * @brief Loads game and user data from a JSON file.
 * * Also attempts to load the API key.
 */
void LoadGamesDataFromJson();
} // namespace loader

STEAM_END_NAMESPACE

#endif