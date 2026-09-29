#include "playerboundingrect.h"

PlayerBoundingRect::PlayerBoundingRect()
{
}

void PlayerBoundingRect::setPlayerItem(PlayerItem* item)
{
   _player_item = item;
}

PlayerItem* PlayerBoundingRect::getPlayerItem() const
{
   return _player_item;
}
