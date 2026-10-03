#ifndef PLAYERBOUNDINGRECT_H
#define PLAYERBOUNDINGRECT_H

// base
#include "tools/rect.h"

#include <cstdint>

class PlayerBoundingRect : public Rect
{
public:
   PlayerBoundingRect();

   void setPlayerId(int32_t id);

   int32_t getPlayerId() const;

protected:
   int32_t _player_id = -1;
};

#endif  // PLAYERBOUNDINGRECT_H
