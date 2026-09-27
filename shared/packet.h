#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "binaryreader.h"
#include "binarywriter.h"

// TODO: public inheritance from std::vector<uint8_t> is a design smell (non-polymorphic base,
// no protection against slicing via the container interface) and TYPE below should really be an
// enum class - both are deferred: TYPE alone has ~226 unqualified call sites (Packet::BOMB etc.)
// across client/, server/ and ai/, well outside shared/'s scope. Revisit in the client/server pass.
class Packet : public std::vector<uint8_t>
{
public:
   //! packet types available
   enum TYPE
   {
      INVALID,
      BOMB,
      COUNTDOWN,
      CREATEGAMEREQUEST,
      CREATEGAMERESPONSE,
      DETONATION,
      ERROR,
      EXTRAMAPITEMCREATED,
      GAMEEVENT,
      GAMESTATS,
      JOINGAMEREQUEST,
      JOINGAMERESPONSE,
      KEY,
      LEAVEGAMEREQUEST,
      LEAVEGAMERESPONSE,
      LISTGAMESREQUEST,
      LISTGAMESRESPONSE,
      LOGINREQUEST,
      LOGINRESPONSE,
      MAPCREATEREQUEST,
      MAPITEMCREATED,
      MAPITEMDESTROYED,
      MAPITEMREMOVED,
      MESSAGE,
      MAPITEMMOVE,
      PLAYERKILLED,
      PLAYERMODIFIED,
      POSITION,
      STARTGAMEREQUEST,
      STARTGAMERESPONSE,
      STOPGAMEREQUEST,
      STOPGAMERESPONSE,
      TIME,
      STONEDROP,
      EXTRASHAKE,
      PLAYERSYNCHRONIZEPACKET,
      PLAYERINFECTEDPACKET
   };

   //! read constructor
   Packet();

   //! write constructor
   Packet(TYPE packetType);

   //! destructor
   virtual ~Packet();

   //! serializes a packet
   void serialize();

   //! deserializes a packet
   static std::unique_ptr<Packet> deserialize(BinaryReader&);

   //! debug function
   virtual void debug() = 0;

   //! enqueue member variables
   virtual void enqueue(BinaryWriter&) = 0;

   //! dequeue member variables
   virtual void dequeue(BinaryReader&) = 0;

   //! getter for packet size
   [[nodiscard]] int16_t getSize();

   //! getter for packet type
   [[nodiscard]] TYPE getType();

   //! getter for the packet's timestamp (milliseconds since midnight)
   [[nodiscard]] int32_t getTimestamp() const;

   //! timestamp can be modified (e.g. in playback)
   void setTimeStamp(int32_t time);

   //! getter for packet name
   [[nodiscard]] const std::string& getPacketName() const;

   //! raw byte pointer, kept for existing socket-write call sites
   [[nodiscard]] const char* constData() const;

protected:
   //! packet size
   int16_t mPacketSize;

   //! packet type
   TYPE mPacketType;

   //! timestamp (milliseconds since midnight)
   int32_t mTimestamp;

   //! packet name
   std::string mPacketName;
};
