#include "playerboundingrect.h"

PlayerBoundingRect::PlayerBoundingRect()
{
}

void PlayerBoundingRect::setPlayerId(int32_t id)
{
   _player_id = id;
}

int32_t PlayerBoundingRect::getPlayerId() const
{
   return _player_id;
}
