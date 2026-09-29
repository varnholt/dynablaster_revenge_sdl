#include "botbombaction.h"

//-----------------------------------------------------------------------------
/*!
*/
BotBombAction::BotBombAction()
{
   _action_type = ActionBomb;
}


bool BotBombAction::isCombinable() const
{
   return false;
}


