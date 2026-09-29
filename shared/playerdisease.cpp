#include "playerdisease.h"

#include "player.h"
#include "random.h"
#include "timer.h"

std::unordered_set<Constants::SkullType> PlayerDisease::_supported_skulls;
std::vector<Constants::SkullType> PlayerDisease::_cube_faces;

PlayerDisease::~PlayerDisease()
{
   for (const auto& callback : _destroy_callbacks)
   {
      callback();
   }
}

void PlayerDisease::addDestroyCallback(std::function<void()> callback)
{
   _destroy_callbacks.push_back(std::move(callback));
}

void PlayerDisease::setType(Constants::SkullType type)
{
   _type = type;
}

Constants::SkullType PlayerDisease::getType() const
{
   return _type;
}

void PlayerDisease::setDuration(int32_t duration)
{
   _duration = duration;
}

int32_t PlayerDisease::getDuration() const
{
   return _duration;
}

bool PlayerDisease::isActive() const
{
   return _active_time.elapsed() < getDuration();
}

void PlayerDisease::activate()
{
   _active_time.start();

   // a disease replaced by a new infection (or destroyed with its player) before the duration
   // elapses must not be aborted through a dangling pointer
   Timer::singleShot(
      getDuration(),
      [this, token = std::weak_ptr<bool>(_lifetime_token)]()
      {
         if (!token.expired())
         {
            abort();
         }
      }
   );
}

void PlayerDisease::randomizeType()
{
   setType(static_cast<Constants::SkullType>(Random::bounded(static_cast<int32_t>(Constants::SkullReset))));
}

void PlayerDisease::applyAutofire(int8_t& keys_pressed)
{
   keys_pressed |= Constants::KeyBomb;
}

void PlayerDisease::applyKeyboardInvert(int8_t& keys_pressed)
{
   int8_t inverted_keys = 0;

   // pass bomb bit
   if (keys_pressed & Constants::KeyBomb)
   {
      inverted_keys |= Constants::KeyBomb;
   }

   // invert others
   if (keys_pressed & Constants::KeyUp)
   {
      inverted_keys |= Constants::KeyDown;
   }
   if (keys_pressed & Constants::KeyDown)
   {
      inverted_keys |= Constants::KeyUp;
   }
   if (keys_pressed & Constants::KeyLeft)
   {
      inverted_keys |= Constants::KeyRight;
   }
   if (keys_pressed & Constants::KeyRight)
   {
      inverted_keys |= Constants::KeyLeft;
   }

   keys_pressed = inverted_keys;
}

int32_t PlayerDisease::getPlayerId() const
{
   return _player_id;
}

void PlayerDisease::setPlayerId(int32_t player_id)
{
   _player_id = player_id;
}

void PlayerDisease::setSupportedSkulls(const std::unordered_set<Constants::SkullType>& skulls)
{
   _supported_skulls = skulls;
}

std::unordered_set<Constants::SkullType> PlayerDisease::getSupportedSkulls()
{
   return _supported_skulls;
}

void PlayerDisease::setSkullFaces(const std::vector<Constants::SkullType>& faces)
{
   _cube_faces = faces;
}

std::vector<Constants::SkullType> PlayerDisease::getSkullFaces()
{
   return _cube_faces;
}

std::vector<Constants::SkullType> PlayerDisease::generateSkullFaces()
{
   // TODO: really generate random faces, for now the setup defined in the server settings is used
   return _cube_faces;
}

void PlayerDisease::abort()
{
   stoppedSignal();
}
