#ifndef BOTBOMBMAPITEM_H
#define BOTBOMBMAPITEM_H

// base
#include "bombmapitem.h"

#include <chrono>

class BotBombMapItem : public BombMapItem
{
   public:

      //! constructor
      BotBombMapItem(
         int player_id,
         int flames,
         int id,
         int x,
         int y
      );

      //! getter for drop time
      std::chrono::steady_clock::time_point getDropTime() const;

      //! setter for flame count
      void setFlameCount(int flames);


   protected:

       //! bomb drop time
       std::chrono::steady_clock::time_point _drop_time;
};

#endif // BOTBOMBMAPITEM_H
