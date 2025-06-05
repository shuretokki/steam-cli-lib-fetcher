// src/steam/undo.cpp
#include "steam/undo.hpp"

#include "steam/data.hpp"  // For fmt
#include "steam/graph.hpp" // For graph::RemoveRelation and graph::SaveRelations

#include <vector> // For managing stack size if kMaxUndoHistory is enforced strictly

STEAM_BEGIN_NAMESPACE
namespace undo {
std::stack<UndoAction> steam_undo_stack;

void PushAddRelationAction(int app_id1, int app_id2)
{
        // If the stack is full, remove the oldest element to make space.
        // std::stack doesn't directly support this, so we'd need to pop all,
        // remove the bottom, then push back. Or use std::deque.
        // For simplicity, if kMaxUndoHistory is a strict limit,
        // we could convert to vector, pop front, then rebuild stack.
        // Or, just let it grow and rely on program restart if memory is a concern.
        // Current implementation will just push. If a strict limit is needed:
        if (steam_undo_stack.size() >= kMaxUndoHistory && kMaxUndoHistory > 0) {
                // Simplest: don't add more if full, or pop one to make space
                // To pop the oldest (bottom) from std::stack:
                std::vector<UndoAction> temp_vec;
                while (!steam_undo_stack.empty()) {
                        temp_vec.push_back(steam_undo_stack.top());
                        steam_undo_stack.pop();
                }
                if (!temp_vec.empty()) {
                        temp_vec.erase(temp_vec.begin()); // Removes the oldest (which was at bottom of stack)
                }
                for (auto it = temp_vec.rbegin(); it != temp_vec.rend(); ++it) {
                        steam_undo_stack.push(*it);
                }
        }
        steam_undo_stack.push({ ActionType::ADD_RELATION, app_id1, app_id2 });
}

bool PopAndExecuteUndo()
{
        if (steam_undo_stack.empty()) {
                fmt::print(fmt::fg(fmt::color::yellow), "Nothing to undo.\n");
                return false;
        }
        UndoAction last_action = steam_undo_stack.top();
        steam_undo_stack.pop();

        switch (last_action.type) {
        case ActionType::ADD_RELATION:
                graph::RemoveRelation(last_action.param1, last_action.param2);
                graph::SaveRelations(); // Save changes after undo
                fmt::print(
                    fmt::fg(fmt::color::light_green),
                    "Successfully undone the relation between AppID {} and AppID {}.\n",
                    last_action.param1,
                    last_action.param2);
                break;
        default:
                fmt::print(fmt::fg(fmt::color::indian_red), "Unknown action type to undo.\n");
                return false;
        }
        return true;
}
} // namespace undo
STEAM_END_NAMESPACE