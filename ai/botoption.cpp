#include "botoption.h"

/*!
   \param score score to set
*/
void BotOption::setScore(int score)
{
   _score = score;
}

/*!
   \return score
*/
int BotOption::getScore() const
{
   return _score;
}

/*!
   \param action bot action
*/
void BotOption::setAction(std::unique_ptr<BotAction> action)
{
   _action = std::move(action);
}

/*!
   \return action
*/
BotAction* BotOption::getAction() const
{
   return _action.get();
}

/*!
   \param combinable combinable flag
*/
void BotOption::setCombinable(bool combinable)
{
   _combinable = combinable;
}

/*!
   \return combinable state
*/
bool BotOption::isCombinable() const
{
   return _combinable;
}
