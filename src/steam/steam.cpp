#include "steam/steam.hpp"

STEAM_BEGIN_NAMESPACE
bool                        steam_has_fetched_data = false;
std::string                 steam_api_key;
std::vector<data::GameData> steam_game_collection;
data::UserData              steam_current_user_data;
std::deque<std::string>     steam_command_history;

namespace prefix {
prefix::PrefixTree                      steam_game_name_prefix_tree;
std::unordered_map<std::string, size_t> steam_game_name_to_index_map;
} // namespace prefix
STEAM_END_NAMESPACE