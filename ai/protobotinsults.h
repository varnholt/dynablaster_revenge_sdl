#ifndef PROTOBOTINSULTS_H
#define PROTOBOTINSULTS_H

#include <string>
#include <vector>

#include "gamesignal.h"

// never actually instantiated in this port (ProtoBot::_insults stays null) - kept for parity
// with the original.
class ProtoBotInsults
{
public:
   ProtoBotInsults();

   Signal<const std::string&> sendMessageSignal;

protected:
   void shootAgain();
   void insult();

   std::vector<std::string> _insults;
};

#endif  // PROTOBOTINSULTS_H
