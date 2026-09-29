#ifndef BOTOPTION_H
#define BOTOPTION_H

#include "botaction.h"


class BotOption
{
   public:

      //! constructor
      BotOption();

      //! destructor
      virtual ~BotOption();

      //! setter for score
      void setScore(int);

      //! getter for score
      int getScore() const;

      //! setter for action
      void setAction(BotAction*);

      //! getter for action
      BotAction* getAction() const;

      //! setter for combinable flag
      void setCombinable(bool combinable);

      //! action is combinable (default is nope)
      virtual bool isCombinable() const;


   protected:

      //! options action
      BotAction* _action;

      //! option's score
      int _score;

      //! combinable flag
      bool _combinable;

};

#endif // BOTOPTION_H
