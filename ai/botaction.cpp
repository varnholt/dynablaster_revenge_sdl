#include "botaction.h"

BotAction::BotAction(ActionType action_type) : _action_type(action_type)
{
}

BotAction::ActionType BotAction::getActionType() const
{
   return _action_type;
}
