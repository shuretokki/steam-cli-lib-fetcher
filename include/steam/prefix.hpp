#ifndef STEAM_DATA_PREFIX_HPP
#define STEAM_DATA_PREFIX_HPP

#include <algorithm> // For std::sort
#include <unordered_map>
#include <vector> // For std::vector
#include "data.hpp"
#include "utility.hpp" // For ToLower

STEAM_BEGIN_NAMESPACE
namespace prefix {

/**
 * @brief Structure for a prefix tree (Trie) to enable efficient prefix - based* game name searches.*/
struct PrefixTree
{
        struct Node
        {
                std::unordered_map<char, Node*> children;
                std::vector<size_t>             game_indices; /* * Indices into steam_game_collection */

                Node()                       = default;

                /* ! Non-copyable and non-movable to simplify memory management. */
                Node(const Node&)            = delete;
                Node& operator=(const Node&) = delete;
                Node(Node&&)                 = delete;
                Node& operator=(Node&&)      = delete;

                ~Node()
                {
                        /* * Recursively delete child nodes. */
                        /* * This is a simple destructor; for complex scenarios, consider smart pointers. */
                        for (auto& pair : children) {
                                delete pair.second;
                        }
                }
        };

        Node* root_node_;

        PrefixTree() : root_node_(new Node())
        {
        }

        ~PrefixTree()
        {
                delete root_node_; /* * This will trigger recursive deletion if Node's destructor is correctly
                                      implemented. */
                root_node_ = nullptr;
        }

        /* * Clears all nodes in the prefix tree.
         * ? Is explicitly calling Clear necessary if destructor handles it?
         * * It can be useful for re-initializing without destruction/reconstruction.
         */
        void Clear()
        {
                delete root_node_;
                root_node_ = new Node();
        }

        /**
         * @brief Inserts a game name into the prefix tree.
         * @param name The name of the game to insert.
         * @param game_index The index of the game in the g_game_collection vector.
         */
        void Insert(const std::string& name, size_t game_index);

        /**
         * @brief Searches for games whose names start with the given prefix.
         * @param prefix The prefix to search for.
         * @return A vector of indices (into g_game_collection) of games matching the
         * prefix.
         */
        std::vector<size_t> SearchByPrefix(const std::string& prefix) const;

      private:
        /**
         * @brief Recursively collects all game indices from a given node and its
         * descendants.
         * @param current_node The current node to process.
         * @param indices A reference to the vector where collected indices will be
         * stored.
         */
        void CollectGameIndicesRecursive(const Node* current_node, std::vector<size_t>& indices) const;
};

/* * Global prefix tree for game name searching. */
extern PrefixTree steam_game_name_prefix_tree;
/* * Global map from lowercase game name to its index in steam_game_collection. */
extern std::unordered_map<std::string, size_t> steam_game_name_to_index_map;
} // namespace prefix
STEAM_END_NAMESPACE

#endif