// src/steam/graph.cpp
#include "steam/graph.hpp"

#include "steam/utility.hpp" // For GetGamesDataPath or similar for relations file
// #include "steam/loader.hpp"  // For nlohmann::json, already included via data.hpp->base.hpp
#include "steam/data.hpp" // For kDataDirectory, fmt, nlohmann::json

#include <algorithm> // for std::remove, std::min etc.
#include <fstream>

STEAM_BEGIN_NAMESPACE
namespace graph {
std::unordered_map<int, std::unordered_set<int>> steam_game_relations_graph;

std::filesystem::path GetRelationsDataPath()
{
        std::filesystem::path data_dir_path = kDataDirectory; // from data.hpp
        if (!std::filesystem::exists(data_dir_path)) {
                std::filesystem::create_directories(data_dir_path);
        }
        return data_dir_path / kRelationsJsonFile;
}

void AddRelation(int app_id1, int app_id2)
{
        if (app_id1 == app_id2)
                return; // Cannot relate a game to itself
        steam_game_relations_graph[app_id1].insert(app_id2);
        steam_game_relations_graph[app_id2].insert(app_id1);
}

void RemoveRelation(int app_id1, int app_id2)
{
        if (steam_game_relations_graph.count(app_id1)) {
                steam_game_relations_graph[app_id1].erase(app_id2);
                if (steam_game_relations_graph[app_id1].empty()) {
                        steam_game_relations_graph.erase(app_id1);
                }
        }
        if (steam_game_relations_graph.count(app_id2)) {
                steam_game_relations_graph[app_id2].erase(app_id1);
                if (steam_game_relations_graph[app_id2].empty()) {
                        steam_game_relations_graph.erase(app_id2);
                }
        }
}

std::vector<int> GetRelatedGames(int app_id, int max_recommendations)
{
        std::vector<int> related;
        if (steam_game_relations_graph.count(app_id)) {
                for (int related_app_id : steam_game_relations_graph.at(app_id)) {
                        if (related.size() >= static_cast<size_t>(max_recommendations))
                                break;
                        related.push_back(related_app_id);
                }
        }
        return related;
}

void SaveRelations()
{
        std::filesystem::path relations_file_path = GetRelationsDataPath();
        std::ofstream         ofs(relations_file_path);

        if (!ofs.is_open()) {
                fmt::print(
                    fmt::fg(fmt::color::indian_red),
                    "Error: Could not open {} for writing.\n",
                    relations_file_path.string());
                return;
        }

        nlohmann::json json_output;
        for (const auto& pair : steam_game_relations_graph) {
                json_output[std::to_string(pair.first)] = pair.second;
        }
        ofs << json_output.dump(4);
        ofs.close();
}

void LoadRelations()
{
        std::filesystem::path relations_file_path = GetRelationsDataPath();
        if (!std::filesystem::exists(relations_file_path)) {
                return; // No relations file yet, that's fine.
        }

        std::ifstream ifs(relations_file_path);
        if (!ifs.is_open()) {
                // Silently return if not readable, or print a warning
                // fmt::print(fmt::fg(fmt::color::yellow), "Warning: Could not open {} for reading.\n",
                // relations_file_path.string());
                return;
        }

        try {
                nlohmann::json json_input;
                ifs >> json_input;
                ifs.close();

                steam_game_relations_graph.clear();
                for (auto it = json_input.begin(); it != json_input.end(); ++it) {
                        try {
                                int                     app_id      = std::stoi(it.key());
                                std::unordered_set<int> related_ids = it.value().get<std::unordered_set<int>>();
                                steam_game_relations_graph[app_id]  = related_ids;
                        } catch (const std::invalid_argument& iae) {
                                fmt::print(
                                    fmt::fg(fmt::color::indian_red),
                                    "Error: Invalid AppID format '{}' in relations file.\n",
                                    it.key());
                        } catch (const std::out_of_range& oor) {
                                fmt::print(
                                    fmt::fg(fmt::color::indian_red),
                                    "Error: AppID '{}' out of range in relations file.\n",
                                    it.key());
                        }
                }
        } catch (const nlohmann::json::exception& e) {
                fmt::print(
                    fmt::fg(fmt::color::indian_red),
                    "Error parsing JSON from {}: {}.\n",
                    relations_file_path.string(),
                    e.what());
                if (ifs.is_open()) {
                        ifs.close();
                }
        }
}
} // namespace graph
STEAM_END_NAMESPACE