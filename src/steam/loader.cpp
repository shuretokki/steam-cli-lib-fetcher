#include "steam/loader.hpp"

#include "steam/prefix.hpp"
#include "steam/utility.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace fmt;
using json = nlohmann::json;

STEAM_BEGIN_NAMESPACE

namespace loader {

void SaveGamesDataToJson()
{
        std::filesystem::path data_file_path = GetGamesDataPath();
        std::ofstream         ofs(data_file_path);

        if (!ofs.is_open()) {
                print(fg(color::indian_red), "Error: Could not open {} for writing.\n", data_file_path.string());
                return;
        }

        json json_output;
        json_output["user"]["username"] = steam_current_user_data.username;
        json_output["user"]["location"] = steam_current_user_data.location;
        json_output["user"]["steam_id"] = steam_current_user_data.steam_id;

        for (const auto& game : steam_game_collection) {
                json game_json;
                game_json["name"]             = game.name;
                game_json["app_id"]           = game.app_id;
                game_json["playtime_forever"] = game.playtime_forever;
                json_output["games"].push_back(game_json);
        }
        ofs << json_output.dump(4);
        ofs.close();
}

void LoadGamesDataFromJson()
{
        if (!api_key::LoadApiKeyFromEnv() && steam_api_key.empty()) {
                print(fg(color::yellow), "STEAM_API_KEY not found or invalid in .env file or environment.\n");
                while (true) {
                        print(
                            fg(color::gold),
                            "Please enter your STEAM API KEY (or type 'skip' to continue without): ");
                        std::string temp_key;
                        if (!std::getline(std::cin, temp_key)) {
                                print(fg(color::yellow), "\nInput error or EOF detected. Cannot read API key.\n");
                                break;
                        }
                        if (temp_key.empty()) {
                                print(fg(color::indian_red), "No API key entered. Please try again or type 'skip'.\n");
                                continue;
                        }
                        if (ToLower(temp_key) == "skip") {
                                print(fg(color::yellow), "API key entry skipped. 'fetch' command will not work.\n");
                                break;
                        }
                        if (api_key::isSteamAPIKeyValid(temp_key)) {
                                steam_api_key = temp_key;
                                std::ofstream ofs_env(".env", std::ios::app);
                                if (ofs_env.is_open()) {
                                        ofs_env << "STEAM_API_KEY=" << steam_api_key << "\n";
                                        ofs_env.close();
                                        print(fg(color::light_green), "API Key saved to .env file.\n");
                                } else {
                                        print(
                                            fg(color::yellow),
                                            "Warning: Could not open .env file to save API key.\n");
                                }
                                break;
                        } else {
                                print(
                                    fg(color::indian_red),
                                    "The API key you entered is invalid. Please try again or type 'skip'.\n");
                        }
                }
        }
        if (steam_api_key.empty()) {
                print(
                    fg(color::yellow),
                    "Warning: API key not loaded. 'fetch' command will not work until key is set.\n");
        }

        std::filesystem::path data_file_path = GetGamesDataPath();
        if (!std::filesystem::exists(data_file_path)) {
                return;
        }

        std::ifstream ifs(data_file_path);
        if (!ifs.is_open()) {
                print(fg(color::indian_red), "Error: Could not open {} for reading.\n", data_file_path.string());
                return;
        }

        try {
                json json_input;
                ifs >> json_input;
                ifs.close();

                if (json_input.contains("user")) {
                        steam_current_user_data.username = json_input["user"].value("username", "N/A");
                        steam_current_user_data.location = json_input["user"].value("location", "N/A");
                        steam_current_user_data.steam_id = json_input["user"].value("steam_id", "");
                        if (!steam_current_user_data.steam_id.empty()) {
                                steam_has_fetched_data = true;
                        }
                }

                if (json_input.contains("games")) {
                        steam_game_collection.clear();
                        prefix::steam_game_name_prefix_tree.Clear();
                        prefix::steam_game_name_to_index_map.clear();
                        size_t current_index = 0;
                        for (const auto& game_json : json_input["games"]) {
                                data::GameData game;
                                game.name             = game_json.value("name", "Unknown Game");
                                game.app_id           = game_json.value("app_id", 0);
                                game.playtime_forever = game_json.value("playtime_forever", 0);
                                steam_game_collection.push_back(game);
                                prefix::steam_game_name_prefix_tree.Insert(game.name, current_index);
                                prefix::steam_game_name_to_index_map[ToLower(game.name)] = current_index;
                                current_index++;
                        }
                }
                if (steam_has_fetched_data) {
                        print(
                            fg(color::light_green),
                            "Loaded {} games for user {} from {}.\n",
                            steam_game_collection.size(),
                            steam_current_user_data.username,
                            data_file_path.string());
                }

        } catch (const json::exception& e) {
                print(fg(color::indian_red), "Error parsing JSON from {}: {}.\n", data_file_path.string(), e.what());
                if (ifs.is_open()) {
                        ifs.close();
                }
        }
}

} // namespace loader
STEAM_END_NAMESPACE