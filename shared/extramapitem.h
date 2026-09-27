#pragma once

// base
#include "mapitem.h"

// shared
#include "constants.h"
#include "elapsedtimer.h"

#include <vector>

// forward declarations
class ExtraMapItemCreatedPacket;

class ExtraMapItem : public MapItem
{
   public:

      //! constructor
      ExtraMapItem(
         int32_t id,
         Constants::ExtraType type,
         int32_t x,
         int32_t y
      );

      //! construct from packet
      ExtraMapItem(ExtraMapItemCreatedPacket* packet);

      //! destructor
      virtual ~ExtraMapItem();

      //! getter for extra type
      [[nodiscard]] Constants::ExtraType getExtraType() const;

      //! initialize start time
      void initializeStartTime();

      //! getter elapsed time
      [[nodiscard]] float getElapsedTime() const;

      //! setter for skull faces
      void setSkullFaces(const std::vector<Constants::SkullType>& faces);

      //! getter for skull faces
      [[nodiscard]] std::vector<Constants::SkullType> getSkullFaces() const;


   private:

      //! extra type
      Constants::ExtraType mExtraType;

      //! time since extra map item is visible
      ElapsedTimer mStartTime;

      //! skull face setup
      std::vector<Constants::SkullType> mSkullFaces;
};
