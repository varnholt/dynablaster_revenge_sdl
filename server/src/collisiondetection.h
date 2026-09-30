#ifndef COLLISIONDETECTION_H
#define COLLISIONDETECTION_H

#include <array>
#include <cstdint>

// shared
#include "constants.h"
#include "gamesignal.h"

// forward declarations
class Game;
class Map;
class MapItem;
class Player;

class CollisionDetection
{
public:
   //! process player
   void process(Player* player);

   //! returns true if position is blocked
   bool isFieldBlocked(const std::array<int32_t, 2>& field, MapItem** blocking_item = nullptr);

   //! getter for game
   Game* getGame() const;

   //! setter for game
   void setGame(Game* game);

   //! player kicks a bomb
   Signal<Player*, MapItem*, bool, int> playerKicksBombSignal;

   //! send an idle packet
   Signal<int8_t, Player*> playerIdleSignal;

   //! move player
   Signal<Player*, float, float, int8_t> playerMoveSignal;

   //! player position was updated
   Signal<int, float, float> playerPositionChangedSignal;

protected:
   //! getter for game
   Map* getMap() const;

   //! prepare referenced values for current player
   void updatePlayerDirections(Player* player, int keys_pressed, int8_t& directions, float& desired_x_position, float& desired_y_position);

   //! adjust player x position
   float adjustXPosition(
      Player* player,
      float x_in_field,
      float desired_x_position,
      bool player_moves_horizontally,
      bool horizontal_movement_forbidden
   );

   //! adjust player y position
   float adjustYPosition(
      Player* player,
      float y_in_field,
      float desired_y_position,
      bool player_moves_vertically,
      bool vertical_movement_forbidden
   );

   //! update the player's rotation
   bool updateRotation(Player* player, int keys_pressed, bool moved, float assigned_x_position, float assigned_y_position);

   //! returns true if position is blocked
   bool
   isPositionBlocked(float x, float y, int keys_pressed, bool vertical_check, int field_x, int field_y, MapItem** blocking_item = nullptr);

   //! game
   Game* _game = nullptr;
};

#endif  // COLLISIONDETECTION_H
