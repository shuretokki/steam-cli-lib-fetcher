#ifndef STEAM_GRAPH_HPP
#define STEAM_GRAPH_HPP

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "data.hpp"

STEAM_BEGIN_NAMESPACE
namespace graph {
// Adjacency list for game relations (appid -> set of related appids)
extern std::unordered_map<int, std::unordered_set<int>> steam_game_relations_graph;
const std::string                                       kRelationsJsonFile = "relations.json";

void                  AddRelation(int app_id1, int app_id2);
void                  RemoveRelation(int app_id1, int app_id2); // Added for undo functionality
std::vector<int>      GetRelatedGames(int app_id, int max_recommendations = 5);
std::filesystem::path GetRelationsDataPath();
void                  LoadRelations(); // Load from a file (e.g., data/relations.json)
void                  SaveRelations(); // Save to a file
} // namespace graph
STEAM_END_NAMESPACE

#endif