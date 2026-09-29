#include "botbombaction.h"

BotBombAction::BotBombAction() : BotAction(ActionType::ActionBomb)
{
}

bool BotBombAction::isCombinable() const
{
   return false;
}
