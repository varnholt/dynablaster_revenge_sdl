#ifndef PLAYERBOUNDINGRECT_H
#define PLAYERBOUNDINGRECT_H

// base
#include "tools/rect.h"

// forward declarations
class PlayerItem;

class PlayerBoundingRect : public Rect
{

public:

   PlayerBoundingRect();

   void setPlayerItem(PlayerItem* item);

   PlayerItem* getPlayerItem() const;


protected:

   PlayerItem* _player_item = nullptr;
};

#endif // PLAYERBOUNDINGRECT_H
