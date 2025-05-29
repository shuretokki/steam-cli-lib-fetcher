#ifndef STEAM_UTILITY_HPP
#define STEAM_UTILITY_HPP
#include <filesystem>
#include <string>
#include "base.hpp"

STEAM_BEGIN_NAMESPACE
/** -----------------------------------------------------------------
 * Helper function to get the full path to the games data JSON file.
 -------------------------------------------------------------------- */
std::filesystem::path GetGamesDataPath();

/** -----------------------------------------------------------------
 * @brief Converts a string to lowercase.
 * @param input_string The string to convert.
 * @return The lowercase version of the input string.
 -------------------------------------------------------------------- */
std::string ToLower(const std::string& input_string);
STEAM_END_NAMESPACE

#endif