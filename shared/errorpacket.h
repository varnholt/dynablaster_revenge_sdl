#pragma once

#include <string>

#include "constants.h"
#include "packet.h"

class ErrorPacket : public Packet
{
public:
   // write constructor
   ErrorPacket(Constants::ErrorType error_type, const std::string& message);

   // read constructor
   ErrorPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] Constants::ErrorType getErrorType() const;
   void setErrorType(Constants::ErrorType error_type);

   [[nodiscard]] const std::string& getErrorMessage() const;
   void setErrorMessage(const std::string& message);

protected:
   Constants::ErrorType _error_type = Constants::ErrorDefault;
   std::string _error_message;
};
