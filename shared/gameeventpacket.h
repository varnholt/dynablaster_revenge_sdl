#pragma once

#include <cstdint>

// shared
#include "constants.h"

// shared
#include "packet.h"

class GameEventPacket : public Packet
{
public:
   //! events
   enum GameEvent
   {
      Invalid,
      BombExploded,
      ExtraCollected,
      ExtraDestroyed
   };

   //! write constructor
   GameEventPacket(GameEvent event, float intensity = 1.0f, int32_t x = -1, int32_t y = -1);

   //! read constructor
   GameEventPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   // BombExplodedPacket

   //! getter for the game event
   [[nodiscard]] GameEvent getGameEvent() const;

   //! getter for the event's intensity
   [[nodiscard]] float getIntensity() const;

   // ExtraCollectedPacket

   //! setter for player id
   void setPlayerId(int32_t);

   //! setter for extra type
   void setExtraType(Constants::ExtraType);

   //! getter for player id
   [[nodiscard]] int32_t getPlayerId() const;

   //! getter for extra type
   [[nodiscard]] Constants::ExtraType getExtraType() const;

   //! getter for x position
   [[nodiscard]] int32_t getX() const;

   //! getter for y position
   [[nodiscard]] int32_t getY() const;

private:
   //! game event
   GameEvent mEvent;

   // TODO: create 2 separate packets here
   //
   // - BombExplodedPacket
   //   +- intensity
   //
   // - ExtraCollectedPacket
   //   +- player id
   //   +- extra type

   //! effect intensity
   float mIntensity;

   // ------- snip --------

   // extra attributes for extra collected

   //! player's id
   int32_t mPlayerId;

   //! extra that was collected
   Constants::ExtraType mExtraType;

   //! affected field x pos
   int32_t mX;

   //! affected field y pos
   int32_t mY;
};
