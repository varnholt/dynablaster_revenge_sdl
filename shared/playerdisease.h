#pragma once

#include <unordered_set>

// constants
#include "constants.h"
#include "elapsedtimer.h"
#include "signal.h"

#include <cstdint>
#include <functional>
#include <vector>

class PlayerDisease
{
public:
   //! constructor
   PlayerDisease();

   //! destructor - runs the destroy callbacks below. Owners (Player/PlayerInfo) hold this via
   //! unique_ptr and destroy it directly on replacement/reset - abort() no longer self-deletes.
   virtual ~PlayerDisease();

   //! run an arbitrary callback right before this object is destroyed - used to disconnect
   //! Signal<> subscriptions registered against a longer-lived signal (Game's own signals live
   //! for the whole match, a disease doesn't), since Signal<> has no automatic
   //! disconnect-on-destroy the way Qt's own connect() did
   void addDestroyCallback(std::function<void()> callback);

   //! setter for disease type
   void setType(Constants::SkullType);

   //! getter for disease type
   [[nodiscard]] Constants::SkullType getType() const;

   //! setter for disease duration
   void setDuration(int32_t duration);

   //! getter for disease duration
   [[nodiscard]] int32_t getDuration() const;

   //! check if disease is still active
   [[nodiscard]] bool isActive() const;

   //! activate disease
   void activate();

   //! set random skull type
   void randomizeType();

   //! getter for player id
   [[nodiscard]] int32_t getPlayerId() const;

   //! setter for player id
   void setPlayerId(int32_t playerId);

   //! setter for supported skulls
   static void setSupportedSkulls(const std::unordered_set<Constants::SkullType>& skulls);

   //! getter for supported skulls;
   [[nodiscard]] static std::unordered_set<Constants::SkullType> getSupportedSkulls();

   //! setter for skull faces
   static void setSkullFaces(std::vector<Constants::SkullType>& faces);

   //! getter for supported skulls;
   [[nodiscard]] static std::vector<Constants::SkullType> getSkullFaces();

   //! generate random skull faces
   [[nodiscard]] static std::vector<Constants::SkullType> generateSkullFaces();

   // skull type implementations

   //! apply autofire
   void applyAutofire(int8_t& keysPressed);

   //! apply keyboard invert
   void applyKeyboardInvert(int8_t& keysPressed);

public:
   //! abort infection
   void abort();

public:
   // Signal<> replacement for PlayerDisease's former Qt signal (see
   // project_full_qt_removal_scope memory).

   //! disease stopped
   Signal<> stoppedSignal;

protected:
   //! disease type
   Constants::SkullType mType;

   //! disease duration
   int32_t mDuration;

   //! active time
   ElapsedTimer mActiveTime;

   //! player id
   int32_t mPlayerId;

   //! all skulls that are supported/enabled
   static std::unordered_set<Constants::SkullType> sSupportedSkulls;

   //! skull cube setup
   static std::vector<Constants::SkullType> sCubeFaces;

   //! run in the destructor - see addDestroyCallback()
   std::vector<std::function<void()>> mDestroyCallbacks;
};
