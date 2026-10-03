#include "extrashakepackethandler.h"

// server
#include "game.h"

// shared
#include "constants.h"
#include "extramapitem.h"
#include "extrashakepacket.h"
#include "logging.h"
#include "map.h"
#include "random.h"
#include "stonemapitem.h"

// stdlib
#include <memory>

ExtraShakePacketHandler::ExtraShakePacketHandler(Game& game) : _game(game)
{
   _check_timer.setInterval(SERVER_SHAKE_CHECK_INTERVAL);
   _check_timer.timeoutSignal.connect([this]() { check(); });
}

void ExtraShakePacketHandler::setEnabled(bool enabled)
{
   if (enabled)
   {
      _check_timer.start();
   }
   else
   {
      _check_timer.stop();
   }
}

Game& ExtraShakePacketHandler::getGame() const
{
   return _game;
}

void ExtraShakePacketHandler::check()
{
   if (getGame().getState() != Constants::GameActive)
   {
      return;
   }

   const Map& map = getGame().getMap();

   const int width = map.getWidth();
   const int height = map.getHeight();

   const int x = Random::bounded(width - 1);
   const int y = Random::bounded(height - 1);

   const auto stone = std::dynamic_pointer_cast<StoneMapItem>(map.getItem(x, y));

   if (stone && stone->hasExtraMapItem())
   {
      getGame().addOutgoingPacket(std::make_unique<ExtraShakePacket>(stone->getUniqueId()));
   }
}
