#ifndef STEAM_DATA_HPP
#define STEAM_DATA_HPP

/**
 * @include deque
 * @brief used for
 */
#include <deque>

/**
 * @include queue
 * @brief used for
 */
#include <queue>

/**
 * @include string
 * @brief used for string usage
 */
#include <string>

/**
 * @include vector
 * @brief used for
 */
#include <vector>

#include "base.hpp"
STEAM_BEGIN_NAMESPACE

namespace data {
struct GameData
{
        std::string name;
        int         app_id;
        int         playtime_forever;
};
} // namespace data

namespace data {
struct UserData
{
        std::string username = "";
        std::string location = "";
        std::string steam_id = "";
};
} // namespace data

/*
 * /// Default directory for storing data.             */
const std::string kDataDirectory         = "data";

/*
 * /// Default filename for storing fetched game data. */
const std::string kGamesDataJsonFile     = "games.json";

/*
 * /// Default directory for exported CSV files.       */
const std::string kExportedDataDirectory = "exported";

/*
 * /// Constants for command history                   */
const size_t kMaxCommandHistorySize      = 20; /*
                                       ! = Max number of commands to store    */

const int kDefaultHistoryDisplayCount    = 10; /*
                                       ! = Default number of commands to show */

/*
 * /// Global variable that check fetched as boolean.        */
extern bool steam_has_fetched_data;
/*
 * /// Global variable storing the Steam API key.            */
extern std::string steam_api_key;

/*
 * /// Global vector storing the list of fetched games.      */
extern std::vector<data::GameData> steam_game_collection;

/*
 * /// Global object storing the current user's information. */
extern data::UserData steam_current_user_data;

/*
 * /// Global deque storing the command history.             */
extern std::deque<std::string> steam_command_history;
STEAM_END_NAMESPACE

#endif