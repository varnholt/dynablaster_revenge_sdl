#ifndef EXTRAMAPITEM_H
#define EXTRAMAPITEM_H

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
         int id,
         Constants::ExtraType type,
         int x,
         int y
      );

      //! construct from packet
      ExtraMapItem(ExtraMapItemCreatedPacket* packet);

      //! destructor
      virtual ~ExtraMapItem();

      //! getter for extra type
      Constants::ExtraType getExtraType();

      //! initialize start time
      void initializeStartTime();

      //! getter elapsed time
      float getElapsedTime() const;

      //! setter for skull faces
      void setSkullFaces(const std::vector<Constants::SkullType>& faces);

      //! getter for skull faces
      std::vector<Constants::SkullType> getSkullFaces() const;


   private:

      //! extra type
      Constants::ExtraType mExtraType;

      //! time since extra map item is visible
      ElapsedTimer mStartTime;

      //! skull face setup
      std::vector<Constants::SkullType> mSkullFaces;
};

#endif // EXTRAMAPITEM_H
