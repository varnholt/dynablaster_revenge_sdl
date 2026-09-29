#pragma once

#include "packet.h"

class PlayerSynchronizePacket : public Packet
{
public:
   // packet can be used for multiple purposes
   enum SynchronizeProcess
   {
      LevelLoaded,
      Invalid
   };

   // read constructor
   PlayerSynchronizePacket();

   // write constructor
   explicit PlayerSynchronizePacket(SynchronizeProcess process);

   void setSynchronizeProcess(SynchronizeProcess process);
   [[nodiscard]] SynchronizeProcess getSynchronizeProcess() const;

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

protected:
   SynchronizeProcess _synchronize_process = Invalid;
};
