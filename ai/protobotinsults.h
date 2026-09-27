#ifndef PROTOBOTINSULTS_H
#define PROTOBOTINSULTS_H

#include <string>
#include <vector>

#include "signal.h"

// never actually instantiated in this port (ProtoBot::mInsults stays null) - kept for parity
// with the original, converted mechanically like everything else.
class ProtoBotInsults
{
public:

   ProtoBotInsults();

   Signal<const std::string&> sendMessageSignal;

protected:

   void shootAgain();
   void insult();

   std::vector<std::string> mInsults;

};

#endif // PROTOBOTINSULTS_H
