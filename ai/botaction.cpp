// header
#include "botaction.h"

//-----------------------------------------------------------------------------
/*!
*/
BotAction::BotAction()
   : _action_type(ActionIdle)
{
}


//-----------------------------------------------------------------------------
/*!
*/
BotAction::~BotAction()
{
}


//-----------------------------------------------------------------------------
/*!
   \return action type
*/
BotAction::ActionType BotAction::getActionType() const
{
   return _action_type;
}
