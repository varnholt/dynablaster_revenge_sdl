#pragma once

#include "constants.h"
#include "elapsedtimer.h"
#include "signal.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

class PlayerDisease
{
public:
   PlayerDisease() = default;

   // runs the destroy callbacks; owners (Player/PlayerInfo) hold this via unique_ptr
   virtual ~PlayerDisease();

   // runs right before this object is destroyed - used to disconnect subscriptions registered
   // against longer-lived signals, since Signal<> has no automatic disconnect-on-destroy
   void addDestroyCallback(std::function<void()> callback);

   void setType(Constants::SkullType type);
   [[nodiscard]] Constants::SkullType getType() const;

   void setDuration(int32_t duration);
   [[nodiscard]] int32_t getDuration() const;

   [[nodiscard]] bool isActive() const;

   void activate();

   // set random skull type
   void randomizeType();

   [[nodiscard]] int32_t getPlayerId() const;
   void setPlayerId(int32_t player_id);

   static void setSupportedSkulls(const std::unordered_set<Constants::SkullType>& skulls);
   [[nodiscard]] static std::unordered_set<Constants::SkullType> getSupportedSkulls();

   static void setSkullFaces(const std::vector<Constants::SkullType>& faces);
   [[nodiscard]] static std::vector<Constants::SkullType> getSkullFaces();

   [[nodiscard]] static std::vector<Constants::SkullType> generateSkullFaces();

   // skull type implementations
   void applyAutofire(int8_t& keys_pressed);
   void applyKeyboardInvert(int8_t& keys_pressed);

   // abort infection
   void abort();

   Signal<> stoppedSignal;

protected:
   Constants::SkullType _type = Constants::SkullAutofire;
   int32_t _duration = SERVER_SKULL_DURATION;
   ElapsedTimer _active_time;
   int32_t _player_id = -1;

   // all skulls that are supported/enabled
   static std::unordered_set<Constants::SkullType> _supported_skulls;

   // skull cube setup
   static std::vector<Constants::SkullType> _cube_faces;

   // run in the destructor, see addDestroyCallback()
   std::vector<std::function<void()>> _destroy_callbacks;

   // expires with this object so a pending activate() timeout never touches a destroyed disease
   std::shared_ptr<bool> _lifetime_token = std::make_shared<bool>(true);
};
