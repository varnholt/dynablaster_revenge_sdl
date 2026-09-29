#ifndef BOTCHARACTER_H
#define BOTCHARACTER_H

class BotCharacter
{
public:
   void setCharacter(int extras, int bomb_drop, int attack);

   int getScoreForExtras() const;
   int getScoreForPrepareBombDrop() const;
   int getScoreForAttack() const;

protected:
   int _score_for_escape = 0;
   int _score_for_extras = 0;
   int _score_for_prepare_bomb_drop = 0;
   int _score_for_attack = 0;
};

#endif  // BOTCHARACTER_H
