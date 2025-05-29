#ifndef STEAM_LOADER_API_KEY_HPP
#define STEAM_LOADER_API_KEY_HPP

#include <cstdlib> /* For std::getenv */
#include <dotenv.h>
#include <string>
#include "base.hpp"

STEAM_BEGIN_NAMESPACE

namespace api_key {

/**
 * @brief Check API Key validity.
 * @return True if the API key was valid, false otherwise.
 */
bool isSteamAPIKeyValid(const std::string& key);

/**
 * @brief Loads the Steam API key from the .env file.
 * @return True if the API key was successfully loaded, false otherwise.
 * ! This function is critical for the application to connect to Steam API.
 */
bool LoadApiKeyFromEnv();
} // namespace api_key

STEAM_END_NAMESPACE

#endif