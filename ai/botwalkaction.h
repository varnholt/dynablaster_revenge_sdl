#ifndef BOTWALKACTION_H
#define BOTWALKACTION_H

#include <cstdint>

// bot
#include "botaction.h"

// shared
#include "constants.h"

class BotWalkAction : public BotAction
{
public:
   //! constructor
   BotWalkAction();

   //! setter for walk direction
   void setWalkKeys(int8_t);

   //! getter for walk direction
   int8_t getWalkKeys() const;

protected:
   //! walk direction
   int8_t _walk_keys = 0;
};

#endif  // BOTWALKACTION_H
