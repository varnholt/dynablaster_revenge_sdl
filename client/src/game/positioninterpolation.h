#ifndef POSITIONINTERPOLATION_H
#define POSITIONINTERPOLATION_H

// shared
#include "constants.h"
#include "framework/frametimer.h"
#include "gamesignal.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

// forward declarations
class BombermanClient;
class MapItem;
class MapItemAnimation;
class PlayerInfo;

class PositionInterpolation
{
public:
   //! constructor
   PositionInterpolation();

   //! destructor (out-of-line: _map_item_animations owns MapItemAnimation via unique_ptr,
   //! only forward-declared here)
   ~PositionInterpolation();

   //! emit player position
   Signal<int, float, float, float> setPlayerPositionSignal;

   //! emit map item position, by unique id
   Signal<int32_t, float, float, float> setMapItemPositionSignal;

   //! emit player speed
   Signal<int, float, float, float> setPlayerSpeedSignal;

   //! one of the items bounces
   Signal<> bounceSignal;

   //! a mapitem starts movement (bomb is kicked)
   void moveMapItem(const MapItem& item, Constants::Direction dir, float speed, int nominal_x, int nominal_y);

   //! a mapitem is removed (bomb exploded)
   void removeMapItem(const MapItem& item);

   //! enabled/disable timers
   void gameStateChanged();

   //! update positions
   void update();

protected:
   //! interpolate player positions
   void interpolatePlayerPositions(float dt);

   //! interpolate map item positions (only bombs right now)
   void interpolateMapItemPositions(float dt);

   //! last update time
   float _time;

   //! update timer
   FrameTimer _timer;

   //! an animated mapitem at the map position it was created at
   struct AnimatedItem
   {
      int32_t _id = 0;
      int32_t _x = 0;
      int32_t _y = 0;
   };

   //! list of animated mapitems
   std::vector<AnimatedItem> _map_items;

   //! map mapitem id <-> mapitemanimation
   std::unordered_map<int32_t, std::unique_ptr<MapItemAnimation>> _map_item_animations;
};

#endif  // POSITIONINTERPOLATION_H
