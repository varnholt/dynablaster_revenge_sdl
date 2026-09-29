#include "botwalkaction.h"

//-----------------------------------------------------------------------------
/*!
*/
BotWalkAction::BotWalkAction()
   : _walk_keys(0)
{
   _action_type = ActionWalk;
}


//-----------------------------------------------------------------------------
/*!
   \param direction walk direction
*/
void BotWalkAction::setWalkKeys(int8_t direction)
{
   _walk_keys = direction;
}


//-----------------------------------------------------------------------------
/*!
   \return walk direction
*/
int8_t BotWalkAction::getWalkKeys() const
{
   return _walk_keys;
}
