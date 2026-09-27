#ifndef POSITIONINTERPOLATION_H
#define POSITIONINTERPOLATION_H

// shared
#include "constants.h"
#include "framework/frametimer.h"
#include "signal.h"

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

      //! destructor (out-of-line: mMapItemAnimations owns MapItemAnimation via unique_ptr,
      //! only forward-declared here)
      ~PositionInterpolation();

      //! emit player position
      Signal<int, float, float, float> setPlayerPositionSignal;

      //! emit map item position
      Signal<MapItem*, float, float, float> setMapItemPositionSignal;

      //! emit player speed
      Signal<int, float, float, float> setPlayerSpeedSignal;

      //! one of the items bounces
      Signal<> bounceSignal;

      //! a mapitem starts movement (bomb is kicked)
      void moveMapItem(
         MapItem* item,
         Constants::Direction dir,
         float speed,
         int nominalX,
         int nominalY
      );

      //! a mapitem is removed (bomb exploded)
      void removeMapItem(MapItem* item);

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
      float mTime;

      //! update timer
      FrameTimer mTimer;

      //! list of animated mapitems
      std::vector<MapItem*> mMapItems;

      //! map mapitem <-> mapitemanimation
      std::unordered_map<MapItem*, std::unique_ptr<MapItemAnimation>> mMapItemAnimations;
};

#endif // POSITIONINTERPOLATION_H

