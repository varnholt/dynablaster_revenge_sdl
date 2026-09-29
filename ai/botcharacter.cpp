#include "botcharacter.h"

#include <cassert>

void BotCharacter::setCharacter(int extras, int bomb_drop, int attack)
{
   assert(extras > 1);

   _score_for_extras = extras;
   _score_for_prepare_bomb_drop = bomb_drop;
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
