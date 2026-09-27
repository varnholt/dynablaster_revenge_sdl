#pragma once

#include "packet.h"

// shared
#include "gameinformation.h"

// Qt

#include <vector>

class ListGamesResponsePacket : public Packet
{
public:
   //! write constructor
   ListGamesResponsePacket(const std::vector<GameInformation>& games, bool update = false);

   //! read constructor
   ListGamesResponsePacket();

   //! destructor
   virtual ~ListGamesResponsePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game name
   [[nodiscard]] std::vector<GameInformation> getGames() const;

   //! setter for update flag
   void setUpdate(bool update);

   //! getter for update flag
   [[nodiscard]] bool isUpdate() const;

private:
   //! game's name
   std::vector<GameInformation> mGames;

   //! update game information
   bool mUpdate;
};
