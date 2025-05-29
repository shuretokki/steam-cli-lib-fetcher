#include "steam/prefix.hpp"

STEAM_BEGIN_NAMESPACE
namespace prefix {

/* * PrefixTree Implementation */
void PrefixTree::Insert(const std::string& name, size_t game_index)
{
        Node*       current_node = root_node_;
        std::string lower_name   = ToLower(name); /* Store and search in lowercase for case-insensitivity */
        for (char ch : lower_name) {
                if (current_node->children.find(ch) == current_node->children.end()) {
                        current_node->children[ch] = new Node();
                }
                current_node = current_node->children[ch];
        }
        current_node->game_indices.push_back(game_index);
}

std::vector<size_t> PrefixTree::SearchByPrefix(const std::string& prefix) const
{
        Node*       current_node = root_node_;
        std::string lower_prefix = ToLower(prefix);
        for (char ch : lower_prefix) {
                if (current_node->children.find(ch) == current_node->children.end()) {
                        return {}; /* * Prefix not found */
                }
                current_node = current_node->children[ch];
        }

        std::vector<size_t> result_indices;
        CollectGameIndicesRecursive(current_node, result_indices);
        std::sort(result_indices.begin(), result_indices.end());
        return result_indices;
}

void PrefixTree::CollectGameIndicesRecursive(const Node* current_node, std::vector<size_t>& indices) const
{
        if (!current_node) {
                return;
        }

        for (size_t index : current_node->game_indices) {
                indices.push_back(index);
        }
        for (const auto& pair : current_node->children) {
                CollectGameIndicesRecursive(pair.second, indices);
        }
}

} // namespace prefix
STEAM_END_NAMESPACE