#pragma once

#include "packet.h"

#include <cstdint>

//! progress of the story mode
class StoryStatePacket : public Packet
{
public:
   //! story progress
   enum State : int8_t
   {
      StateStageIntro,
      StatePlaying,
      StateStageCleared,
      StateLifeLost,
      StateGameOver,
      StateCompleted
   };

   // read constructor
   StoryStatePacket();

   // write constructor
   StoryStatePacket(int8_t stage, int8_t lives, int32_t score, int8_t state, int8_t enemies_left);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int8_t getStage() const;
   [[nodiscard]] int8_t getLives() const;
   [[nodiscard]] int32_t getScore() const;
   [[nodiscard]] int8_t getState() const;
   [[nodiscard]] int8_t getEnemiesLeft() const;

private:
   int8_t _stage = 0;
   int8_t _lives = 0;
   int32_t _score = 0;
   int8_t _state = 0;
   int8_t _enemies_left = 0;
};
