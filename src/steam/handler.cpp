#include "steam/handler.hpp"
#include <numeric>

using json = nlohmann::json;
using namespace fmt;
STEAM_BEGIN_NAMESPACE

namespace handler {
bool FetchGamesFromSteamApi(const std::string& steam_id_or_vanity_url) // Implementation already here
{

        if (steam_api_key.empty()) {
                print(fg(color::indian_red), "Error: Steam API key is not set. Configure .env file or enter key.\n");
                if (!api_key::LoadApiKeyFromEnv()) { // Attempt to load/prompt again
                        print(fg(color::indian_red), "API key still not available. Fetch aborted.\n");
                        return false;
                }
        }

        httplib::Client steam_api_client("api.steampowered.com");
        steam_api_client.set_connection_timeout(10, 0); // 10s connection timeout
        // steam_api_client.set_follow_location(true); // Good for redirects if any

        std::string resolved_steam_id = steam_id_or_vanity_url;

        // Try to resolve if it's not a 17-digit number (potential vanity URL)
        if (resolved_steam_id.length() != 17
            || !std::all_of(resolved_steam_id.begin(), resolved_steam_id.end(), ::isdigit)) {
                print("Attempting to resolve vanity URL: {}\n", steam_id_or_vanity_url);
                std::string vanity_url_name = steam_id_or_vanity_url; // Assume it's a vanity URL
                std::string resolve_vanity_path =
                    format("/ISteamUser/ResolveVanityURL/v0001/?key={}&vanityurl={}", steam_api_key, vanity_url_name);

                steam_api_client.set_read_timeout(15, 0); // 15s read timeout
                auto response = steam_api_client.Get(resolve_vanity_path.c_str());

                if (!response) {
                        print(
                            fg(color::indian_red),
                            "Error: Failed to connect to Steam API for vanity URL resolution.\n");
                        return false;
                }
                if (response->status != 200) {
                        print(
                            fg(color::indian_red),
                            "Error: Steam API returned status {} for vanity URL resolution.\n",
                            response->status);
                        return false;
                }
                try {
                        json vanity_json = json::parse(response->body);
                        if (vanity_json.contains("response") && vanity_json["response"]["success"] == 1) {
                                resolved_steam_id = vanity_json["response"]["steamid"].get<std::string>();
                                print(
                                    fg(color::light_green),
                                    "Vanity URL resolved to SteamID: {}\n",
                                    resolved_steam_id);
                        } else {
                                print(
                                    fg(color::indian_red),
                                    "Error: Could not resolve vanity URL '{}'. Ensure it's correct or use "
                                    "SteamID64.\n",
                                    vanity_url_name);
                                return false;
                        }
                } catch (const std::exception& e) {
                        print(fg(color::indian_red), "Error: Failed to parse vanity URL response: {}.\n", e.what());
                        return false;
                }
        }

        /* * Fetch player summary */
        print("Fetching player summary for SteamID: {}\n", resolved_steam_id);
        steam_api_client.set_read_timeout(15, 0); // Reset read timeout for next call
        std::string player_summary_path =
            format("/ISteamUser/GetPlayerSummaries/v0002/?key={}&steamids={}", steam_api_key, resolved_steam_id);
        auto summary_response = steam_api_client.Get(player_summary_path.c_str());

        if (!summary_response) {
                print(fg(color::indian_red), "Error: Failed to connect to Steam API for player summaries.\n");
                return false;
        }
        if (summary_response->status != 200) {
                print(
                    fg(color::indian_red),
                    "Error: Steam API returned status {} for player summaries.\n",
                    summary_response->status);
                return false;
        }
        try {
                json summary_json = json::parse(summary_response->body);
                if (!summary_json.contains("response") || !summary_json["response"].contains("players")
                    || summary_json["response"]["players"].empty()) {
                        print(
                            fg(color::indian_red),
                            "Error: No player data found for SteamID {}. Profile might be private or ID "
                            "incorrect.\n",
                            resolved_steam_id);
                        return false;
                }
                auto player_data                 = summary_json["response"]["players"][0];
                steam_current_user_data.steam_id = resolved_steam_id;
                steam_current_user_data.username = player_data.value("personaname", "Unknown User");
                steam_current_user_data.location = format(
                    "{}, {}",
                    player_data.value("locstatecode", "N/A"),
                    player_data.value("loccountrycode", "N/A"));
                if (steam_current_user_data.location == "N/A, N/A")
                        steam_current_user_data.location = "Unknown";

        } catch (const std::exception& e) {
                print(fg(color::indian_red), "Error: Failed to parse user summary: {}.\n", e.what());
                return false; /* Critical error if summary fails */
        }

        /* * Fetch owned games */
        print(
            "Fetching owned games for {} ({})...\n",
            steam_current_user_data.username,
            steam_current_user_data.steam_id);
        steam_api_client.set_read_timeout(30, 0); // Games list can be larger, longer timeout
        std::string owned_games_path = format(
            "/IPlayerService/GetOwnedGames/v0001/"
            "?key={}&steamid={}&format=json&include_appinfo=true", // include_appinfo=1 is fine, true is more
                                                                   // C++ like
            steam_api_key,
            resolved_steam_id);
        auto games_response = steam_api_client.Get(owned_games_path.c_str());

        if (!games_response) {
                print(fg(color::indian_red), "Error: Failed to connect to Steam API for owned games.\n");
                return false;
        }
        if (games_response->status != 200) {
                print(
                    fg(color::indian_red),
                    "Error: Steam API returned status {} for owned games.\n",
                    games_response->status);
                return false;
        }
        try {
                json games_json = json::parse(games_response->body);
                if (!games_json.contains("response") || !games_json["response"].contains("games")) {
                        print(
                            fg(color::yellow),
                            "Warning: No games found in API response or profile might be private.\n");
                        steam_has_fetched_data = true; // User data was fetched
                        steam_game_collection.clear(); // Ensure game list is empty
                        prefix::steam_game_name_prefix_tree.Clear();
                        prefix::steam_game_name_to_index_map.clear();
                        loader::SaveGamesDataToJson(); // Save the user data and empty game list
                        return true;
                }
                auto game_list_json = games_json["response"]["games"];
                if (game_list_json.empty()) {
                        print(
                            fg(color::yellow),
                            "Warning: Game list is empty. User may own no games or profile is private.\n");
                        steam_has_fetched_data = true;
                        steam_game_collection.clear();
                        prefix::steam_game_name_prefix_tree.Clear();
                        prefix::steam_game_name_to_index_map.clear();
                        loader::SaveGamesDataToJson();
                        return true;
                }

                steam_game_collection.clear();
                prefix::steam_game_name_prefix_tree.Clear();
                prefix::steam_game_name_to_index_map.clear();
                steam_has_fetched_data = true;
                size_t current_index   = 0;

                for (const auto& game_entry : game_list_json) {
                        data::GameData game;
                        game.name             = game_entry.value("name", "Unnamed Game");
                        game.app_id           = game_entry.value("appid", 0);
                        game.playtime_forever = game_entry.value("playtime_forever", 0);
                        steam_game_collection.push_back(game);
                        prefix::steam_game_name_prefix_tree.Insert(game.name, current_index);
                        prefix::steam_game_name_to_index_map[ToLower(game.name)] = current_index;
                        current_index++;
                }
                loader::SaveGamesDataToJson();
                print(
                    fg(color::light_green),
                    "Fetched {} games for {}.\n",
                    steam_game_collection.size(),
                    steam_current_user_data.username);
                print(
                    fg(color::yellow),
                    "Profile: https://steamcommunity.com/profiles/{}\n",
                    steam_current_user_data.steam_id);
                return true;

        } catch (const std::exception& e) {
                print(fg(color::indian_red), "Error: Failed to parse owned games response: {}.\n", e.what());
                return false;
        }
}

void HandleSearchCommand(const std::string& name_prefix)
{
        if (steam_game_collection.empty() && !steam_has_fetched_data) {
                print(fg(color::yellow), "No local game data. Use 'fetch <SteamID/VanityURL>' first.\n");
                return;
        }
        if (steam_game_collection.empty() && steam_has_fetched_data) {
                print(
                    fg(color::yellow),
                    "No games found for the current user ({}). Profile might have been private during last fetch.\n",
                    steam_current_user_data.username);
                return;
        }

        auto found_indices = prefix::steam_game_name_prefix_tree.SearchByPrefix(name_prefix);
        if (found_indices.empty()) {
                print(fg(color::indian_red), "No games found matching prefix '{}'.\n", name_prefix);
                return;
        }

        size_t max_name_width = 0;
        for (size_t index : found_indices) {
                if (index < steam_game_collection.size()) {
                        max_name_width = std::max(max_name_width, steam_game_collection[index].name.length());
                }
        }
        max_name_width = std::min(max_name_width + 4, static_cast<size_t>(40));

        print(fg(color::gold) | emphasis::bold, "Search Results for '{}':\n", name_prefix);
        print(fg(color::cyan), "{:<10} {:<{}} {:<15}\n", "AppID", "Name", max_name_width, "Playtime (H:M)");
        print(fg(color::cyan), "{:-<10} {:-<{}} {:-<15}\n", "", "", max_name_width, "");

        for (size_t index : found_indices) {
                if (index < steam_game_collection.size()) {
                        const auto& game         = steam_game_collection[index];
                        std::string display_name = game.name;
                        if (display_name.length() > max_name_width - 3 && max_name_width > 3) {
                                display_name = display_name.substr(0, max_name_width - 3) + "...";
                        }
                        int hours   = game.playtime_forever / 60;
                        int minutes = game.playtime_forever % 60;
                        print(
                            fg(color::white),
                            "{:<10} {:<{}} {:>5}:{:0>2}\n",
                            game.app_id,
                            display_name,
                            max_name_width,
                            hours,
                            minutes);
                }
        }
        print(fg(color::cyan), "--------------------------------------------------\n");
}

void HandleCountPlayedCommand()
{
        if (steam_game_collection.empty() && !steam_has_fetched_data) {
                print(fg(color::yellow), "No local game data. Use 'fetch <SteamID/VanityURL>' first.\n");
                return;
        }
        if (steam_game_collection.empty() && steam_has_fetched_data) {
                print(
                    fg(color::yellow),
                    "No games found for the current user ({}). Profile might have been private during last fetch.\n",
                    steam_current_user_data.username);
                return;
        }

        size_t played_count = 0;
        for (const auto& game : steam_game_collection) {
                if (game.playtime_forever > 0) {
                        played_count++;
                }
        }
        print(fg(color::light_green), "Number of games played: {}\n", played_count);
        print(fg(color::light_green), "Number of games not played: {}\n", steam_game_collection.size() - played_count);
        print(fg(color::light_green), "Total games in library: {}\n", steam_game_collection.size());
}

void HandleExportToCsvCommand(const std::string& output_filename_base)
{
        if (steam_game_collection.empty() && !steam_has_fetched_data) {
                print(fg(color::yellow), "No local game data to export. Use 'fetch' first.\n");
                return;
        }
        if (steam_game_collection.empty() && steam_has_fetched_data) {
                print(
                    fg(color::yellow),
                    "No games found for the current user ({}) to export.\n",
                    steam_current_user_data.username);
                return;
        }

        std::filesystem::path export_dir_path = std::filesystem::path(kDataDirectory) / kExportedDataDirectory;
        if (!std::filesystem::exists(export_dir_path)) {
                std::filesystem::create_directories(export_dir_path);
        }
        std::filesystem::path output_file_path = export_dir_path / (output_filename_base + ".csv");
        std::ofstream         ofs(output_file_path);

        if (!ofs.is_open()) {
                print(fg(color::indian_red), "Error: Could not open {} for writing.\n", output_file_path.string());
                return;
        }

        ofs << "AppID,Name,PlaytimeMinutes\n";
        for (const auto& game : steam_game_collection) {
                std::string csv_name = game.name;
                size_t      pos      = csv_name.find('"');
                while (pos != std::string::npos) {
                        csv_name.replace(pos, 1, "\"\"");
                        pos = csv_name.find('"', pos + 2);
                }
                if (csv_name.find(',') != std::string::npos || csv_name.find('"') != std::string::npos) {
                        csv_name = "\"" + csv_name + "\"";
                }
                ofs << game.app_id << "," << csv_name << "," << game.playtime_forever << "\n";
        }
        ofs.close();
        print(
            fg(color::light_green),
            "Exported {} games to {}.\n",
            steam_game_collection.size(),
            output_file_path.string());
}

static void PrintGameTable(const std::vector<size_t>& indices_to_print, const std::string& title)
{
        if (indices_to_print.empty() && !steam_has_fetched_data && steam_game_collection.empty()) {
                print(fg(color::yellow), "No local game data. Use 'fetch <SteamID/VanityURL>' first.\n");
                return;
        }
        if (indices_to_print.empty() && steam_has_fetched_data && steam_game_collection.empty()) {
                print(
                    fg(color::yellow),
                    "No games found for the current user ({}). Profile might have been private during last fetch.\n",
                    steam_current_user_data.username);
                return;
        }
        if (indices_to_print.empty() && !steam_game_collection.empty()) {
                print(fg(color::yellow), "No games to display for this list type.\n");
                return;
        }

        size_t max_name_width = 0;
        for (size_t index : indices_to_print) {
                if (index < steam_game_collection.size()) {
                        max_name_width = std::max(max_name_width, steam_game_collection[index].name.length());
                }
        }
        max_name_width = std::min(max_name_width + 4, static_cast<size_t>(40));

        print(fg(color::gold) | emphasis::bold, "{}\n", title);
        print(fg(color::cyan), "{:<10} {:<{}} {:<15}\n", "AppID", "Name", max_name_width, "Playtime (H:M)");
        print(fg(color::cyan), "{:-<10} {:-<{}} {:-<15}\n", "", "", max_name_width, "");

        for (size_t index : indices_to_print) {
                if (index < steam_game_collection.size()) {
                        const auto& game         = steam_game_collection[index];
                        std::string display_name = game.name;
                        if (display_name.length() > max_name_width - 3 && max_name_width > 3) {
                                display_name = display_name.substr(0, max_name_width - 3) + "...";
                        }
                        int hours   = game.playtime_forever / 60;
                        int minutes = game.playtime_forever % 60;
                        print(
                            fg(color::white),
                            "{:<10} {:<{}} {:>5}:{:0>2}\n",
                            game.app_id,
                            display_name,
                            max_name_width,
                            hours,
                            minutes);
                }
        }
        print(fg(color::cyan), "--------------------------------------------------\n");
        print(fg(color::light_green), "Displayed {} games.\n", indices_to_print.size());
}

void HandleListGamesCommand(char list_format)
{
        if (steam_game_collection.empty() && !steam_has_fetched_data) {
                print(fg(color::yellow), "No local game data. Use 'fetch <SteamID/VanityURL>' first.\n");
                return;
        }
        if (steam_game_collection.empty() && steam_has_fetched_data) {
                print(
                    fg(color::yellow),
                    "No games found for the current user ({}). Profile might have been private during last fetch.\n",
                    steam_current_user_data.username);
                return;
        }

        std::vector<size_t> indices(steam_game_collection.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                return ToLower(steam_game_collection[a].name) < ToLower(steam_game_collection[b].name);
        });

        switch (list_format) {
        case ' ': {
                print(fg(color::gold) | emphasis::bold, "All Games (Alphabetical):\n");
                size_t max_name_width = 0;
                for (const auto& game : steam_game_collection) {
                        max_name_width = std::max(max_name_width, game.name.length());
                }
                max_name_width = std::min(max_name_width + 2, static_cast<size_t>(50));
                for (size_t index : indices) {
                        std::string display_name = steam_game_collection[index].name;
                        if (display_name.length() > max_name_width - 3 && max_name_width > 3) {
                                display_name = display_name.substr(0, max_name_width - 3) + "...";
                        }
                        print(fg(color::white), "- {:<{}}\n", display_name, max_name_width);
                }
                print(fg(color::light_green), "\nTotal games: {}\n", steam_game_collection.size());
                break;
        }
        case 'l':
                PrintGameTable(indices, "All Games (Alphabetical by Name):");
                break;
        case 'p': {
                std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                        if (steam_game_collection[a].playtime_forever != steam_game_collection[b].playtime_forever) {
                                return steam_game_collection[a].playtime_forever
                                       > steam_game_collection[b].playtime_forever;
                        }
                        return ToLower(steam_game_collection[a].name) < ToLower(steam_game_collection[b].name);
                });
                PrintGameTable(indices, "All Games (Sorted by Playtime):");
                break;
        }
        case 'n': {
                print(fg(color::gold) | emphasis::bold, "Games by Initial Letter:\n");
                char   current_letter = 0;
                size_t max_name_width = 0;
                for (const auto& game : steam_game_collection) {
                        max_name_width = std::max(max_name_width, game.name.length());
                }
                max_name_width = std::min(max_name_width + 4, static_cast<size_t>(40));

                for (size_t index : indices) {
                        const auto& game = steam_game_collection[index];
                        if (game.name.empty())
                                continue;
                        char first_char = std::toupper(game.name[0]);
                        if (!std::isalpha(first_char))
                                first_char = '#';

                        if (first_char != current_letter) {
                                if (current_letter != 0)
                                        print("\n");
                                print(fg(color::cyan) | emphasis::bold, "-- {} --\n", first_char);
                                current_letter = first_char;
                        }
                        std::string display_name = game.name;
                        if (display_name.length() > max_name_width - 3 && max_name_width > 3) {
                                display_name = display_name.substr(0, max_name_width - 3) + "...";
                        }
                        print(fg(color::white), "{:<{}} {:<10}\n", display_name, max_name_width, game.app_id);
                }
                print(fg(color::light_green), "\nTotal games: {}\n", steam_game_collection.size());
                break;
        }
        default:
                print(
                    fg(color::indian_red),
                    "Error: Unknown list format '{}'. Use ' ', 'l', 'n', or 'p'.\n",
                    list_format);
                break;
        }
}

void ShowHelp()
{
        if (steam_has_fetched_data && !steam_current_user_data.steam_id.empty()) {
                print(fg(color::cyan) | emphasis::bold, "\n-- Current Account --\n");
                print(fg(color::white), "Username: {}\n", steam_current_user_data.username);
                print(fg(color::white), "Location: {}\n", steam_current_user_data.location);
                print(fg(color::white), "SteamID:  {}\n", steam_current_user_data.steam_id);
                print(fg(color::white), "Profile:  ");
                print(
                    fg(color::light_blue),
                    "https://steamcommunity.com/profiles/{}/\n",
                    steam_current_user_data.steam_id);
        }
        print(fg(color::cyan) | emphasis::bold, "\n-- Commands --\n");
        print(
            "  fetch <SteamID>       - Fetch game data for a Steam user.\n"
            "  search <prefix>       - Search for games by name prefix.\n"
            "  count                 - Show counts of played/unplayed games.\n"
            "  list                  - Show all game names (alphabetical).\n"
            "  list -l               - Show AppID, name, playtime (name sort).\n"
            "  list -n               - Show name, AppID, grouped by first letter.\n"
            "  list -p               - Show AppID, name, playtime (playtime sort).\n"
            "  export <filename>     - Export games to data/exported/filename.csv.\n"
            "  history [N]           - Show last N commands (default {}).\n"
            "  help                  - Show this help message.\n"
            "  exit                  - Exit the program.\n",
            kDefaultHistoryDisplayCount);
        print(fg(color::cyan), "---------------------\n");
        print(
            fg(color::yellow),
            "  - Ensure STEAM_API_KEY is set in a '.env' file in the same "
            "directory as the executable, or enter it when prompted.\n");
        print(fg(color::yellow), "  - Data is stored in: {}\n\n", GetGamesDataPath().string());
}

} // namespace handler
STEAM_END_NAMESPACE