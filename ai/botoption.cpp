#include "botoption.h"

//-----------------------------------------------------------------------------
/*!
*/
BotOption::BotOption()
   : _action(0),
     _score(0),
     _combinable(false)
{
}


//-----------------------------------------------------------------------------
/*!
*/
BotOption::~BotOption()
{
   delete _action;
   _action = 0;
}


//-----------------------------------------------------------------------------
/*!
   \param score score to set
*/
void BotOption::setScore(int score)
{
   _score = score;
}


//-----------------------------------------------------------------------------
/*!
   \return score
*/
int BotOption::getScore() const
{
   return _score;
}


//-----------------------------------------------------------------------------
/*!
   \param action bot action
*/
void BotOption::setAction(BotAction* action)
{
   _action = action;
}


//-----------------------------------------------------------------------------
/*!
   \return action
*/
BotAction* BotOption::getAction() const
{
   return _action;
}


//-----------------------------------------------------------------------------
/*!
   \param combinable combinable flag
*/
void BotOption::setCombinable(bool combinable)
{
   _combinable = combinable;
}


//-----------------------------------------------------------------------------
/*!
   \return combinable state
*/
bool BotOption::isCombinable() const
{
   return _combinable;
}



