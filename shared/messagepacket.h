#pragma once

#include <string>

#include "packet.h"

class MessagePacket : public Packet
{
public:
   // write constructor
   MessagePacket(int8_t sender_id, const std::string& message, bool finished_typing, int8_t receiver_id = -1);

   // read constructor
   MessagePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int8_t getSenderId() const;
   [[nodiscard]] std::string getMessage() const;
   [[nodiscard]] int8_t getReceiverId() const;
   [[nodiscard]] bool isTypingFinished() const;

private:
   int8_t _sender_id = 0;
   std::string _message;
   int8_t _receiver_id = -1;
   bool _finished_typing = true;
};
