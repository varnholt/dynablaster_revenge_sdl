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

ExtraShakePacketHandler::ExtraShakePacketHandler()
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

void ExtraShakePacketHandler::setGame(Game* game)
{
   _game = game;
}

Game* ExtraShakePacketHandler::getGame() const
{
   return _game;
}

void ExtraShakePacketHandler::check()
{
   if (getGame()->getState() != Constants::GameActive)
   {
      return;
   }

   Map* map = getGame()->getMap();

   if (!map)
   {
      return;
   }

   const int width = map->getWidth();
   const int height = map->getHeight();

   const int x = Random::bounded(width - 1);
   const int y = Random::bounded(height - 1);

   auto* stone = dynamic_cast<StoneMapItem*>(map->getItem(x, y));

   if (stone && stone->getExtraMapItem())
   {
      getGame()->addOutgoingPacket(std::make_unique<ExtraShakePacket>(stone->getUniqueId()));
   }
}
