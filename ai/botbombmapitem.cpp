#include "botbombmapitem.h"

/*!
   \param player_id player id
   \param flames number of flames
   \param id mapitem layout id
   \param x x position
   \param y y position
*/
BotBombMapItem::BotBombMapItem(int player_id, int flames, int id, int x, int y) : BombMapItem(player_id, flames, id, x, y)
{
}

/*!
   \return drop time
*/
std::chrono::steady_clock::time_point BotBombMapItem::getDropTime() const
{
   return _drop_time;
}

/*!
   \param flames flame count
*/
void BotBombMapItem::setFlameCount(int flames)
{
   // BombMapItem has no flame setter, so the protected base member is written directly
   mFlames = static_cast<int8_t>(flames);
}
