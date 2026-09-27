// header
#include "botcharacter.h"

#include <cassert>

//-----------------------------------------------------------------------------
/*!
*/
BotCharacter::BotCharacter()
 : mScoreForEscape(0), 
   mScoreForExtras(0),
   mScoreForPrepareBombDrop(0),
   mScoreForAttack(0)
{
}


void BotCharacter::setCharacter(int extras, int bombdrop, int attack)
{
   assert(extras > 1);

   mScoreForExtras = extras;
   mScoreForPrepareBombDrop = bombdrop;
   mScoreForAttack = attack;
}


int BotCharacter::getScoreForExtras() const
{
   return mScoreForExtras;
}

int BotCharacter::getScoreForPrepareBombDrop() const
{
   return mScoreForPrepareBombDrop;
}

int BotCharacter::getScoreForAttack() const
{
   return mScoreForAttack;
}
