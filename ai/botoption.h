#ifndef BOTOPTION_H
#define BOTOPTION_H

#include "botaction.h"

#include <memory>

class BotOption
{
public:
   //! destructor
   virtual ~BotOption() = default;

   //! setter for score
   void setScore(int);

   //! getter for score
   int getScore() const;

   //! setter for action
   void setAction(std::unique_ptr<BotAction> action);

   //! getter for action, the option must have one
   BotAction& getAction() const;

   //! setter for combinable flag
   void setCombinable(bool combinable);

   //! action is combinable (default is nope)
   virtual bool isCombinable() const;

protected:
   //! options action
   std::unique_ptr<BotAction> _action;

   //! option's score
   int _score = 0;

   //! combinable flag
   bool _combinable = false;
};

#endif  // BOTOPTION_H
