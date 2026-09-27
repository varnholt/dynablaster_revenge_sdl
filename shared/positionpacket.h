#pragma once

#include "packet.h"

class PositionPacket : public Packet
{
public:
   //! write constructor
   PositionPacket(
      int8_t playerId,
      int8_t directions,
      float x,
      float y,
      float angle = 0.0f,
      float xDelta = 0.0f,
      float yDelta = 0.0f,
      float angleDelta = 0.0f,
      float speed = 0.0f
   );

   //! read constructor
   PositionPacket();

   //! destructor
   virtual ~PositionPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for player id
   [[nodiscard]] int8_t getPlayerId() const;

   //! getter for player x position
   [[nodiscard]] float getX() const;

   //! getter for player y position
   [[nodiscard]] float getY() const;

   //! getter for player orientation angle
   [[nodiscard]] float getAngle() const;

   //! getter for the player's directions
   [[nodiscard]] int8_t getDirections() const;

   //! getter for player's x delta
   [[nodiscard]] float getDeltaX() const;

   //! set player's x delta
   void setDeltaX(float deltax);

   //! getter for player's y delta
   [[nodiscard]] float getDeltaY() const;

   //! set player's y delta
   void setDeltaY(float deltay);

   //! getter for player's rotation delta
   [[nodiscard]] float getAngleDelta() const;

   //! getter for player speed
   [[nodiscard]] float getSpeed() const;
   //! set player's rotation delta
   void setAngleDelta(float angleDelta);

private:
   //! player id
   int8_t mPlayerId;

   //! player's directions
   int8_t mDirections;

   //! player x position
   float mX;

   //! player y position
   float mY;

   //! player direction x
   float mDx;

   //! player direction y
   float mDy;

   //! player orientation
   float mAngle;

   //! player angle direction
   float mAngleDelta;

   //! player speed
   float mSpeed;
};
