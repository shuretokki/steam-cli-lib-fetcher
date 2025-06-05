// include/steam/undo.hpp
#ifndef STEAM_UNDO_HPP
#define STEAM_UNDO_HPP

#include <stack>
#include <string> // Required for ActionType if it uses string params later
#include "base.hpp"

STEAM_BEGIN_NAMESPACE
namespace undo {
enum class ActionType
{
        ADD_RELATION,
        // Future actions: EXPORT_FILE etc.
};

struct UndoAction
{
        ActionType type;
        int        param1; // e.g., app_id1 for ADD_RELATION
        int        param2; // e.g., app_id2 for ADD_RELATION
                           // std::string str_param1; // for things like filenames
};

extern std::stack<UndoAction> steam_undo_stack;
const size_t                  kMaxUndoHistory = 10; // Max undo actions to store

void PushAddRelationAction(int app_id1, int app_id2);
bool PopAndExecuteUndo();

} // namespace undo
STEAM_END_NAMESPACE

#endif