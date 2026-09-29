#ifndef BOTCHARACTER_H
#define BOTCHARACTER_H

class BotCharacter
{
   public:

      //! constructor
      BotCharacter();

      void setCharacter(int extras, int bombdrop, int attack);

      int getScoreForExtras() const;
      int getScoreForPrepareBombDrop() const;
      int getScoreForAttack() const;


   protected:

      int _score_for_escape;
      int _score_for_extras;
      int _score_for_prepare_bomb_drop;
      int _score_for_attack;

};

#endif // BOTCHARACTER_H
