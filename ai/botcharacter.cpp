// header
#include "botcharacter.h"

#include <cassert>

//-----------------------------------------------------------------------------
/*!
*/
BotCharacter::BotCharacter()
 : _score_for_escape(0), 
   _score_for_extras(0),
   _score_for_prepare_bomb_drop(0),
   _score_for_attack(0)
{
}


void BotCharacter::setCharacter(int extras, int bombdrop, int attack)
{
   assert(extras > 1);

   _score_for_extras = extras;
   _score_for_prepare_bomb_drop = bombdrop;
   _score_for_attack = attack;
}


int BotCharacter::getScoreForExtras() const
{
   return _score_for_extras;
}

int BotCharacter::getScoreForPrepareBombDrop() const
{
   return _score_for_prepare_bomb_drop;
}

int BotCharacter::getScoreForAttack() const
{
   return _score_for_attack;
}
