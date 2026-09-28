#include "invisibleplayereffect.h"

#include "framework/globaltime.h"
#include "gldevice.h"
#include "materials/invisibilitymaterial.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "playeritem.h"

#include <cstdint>

namespace
{
// the level's player scene holds MAX_PLAYERS player materials, followed by the invisibility material
constexpr int32_t INVISIBILITY_MATERIAL_INDEX = 10;

constexpr float TIME_FADE_IN_START = 1.0f;
constexpr float TIME_FADE_IN_END = 3.0f;
constexpr float TIME_FADE_OUT_START = 17.0f;
constexpr float TIME_DURATION = 20.0f;
constexpr float PARAM_MAX = 2.0f;
}  // namespace

InvisiblePlayerEffect::~InvisiblePlayerEffect()
{
   if (_background_texture)
   {
      glDeleteTextures(1, &_background_texture);
   }
}

void InvisiblePlayerEffect::setPlayerScene(SceneGraph* players)
{
   _scene = players;
}

InvisibilityMaterial* InvisiblePlayerEffect::getMaterial() const
{
   if (!_scene)
   {
      return nullptr;
   }

   return dynamic_cast<InvisibilityMaterial*>(_scene->getMaterial(INVISIBILITY_MATERIAL_INDEX));
}

void InvisiblePlayerEffect::addPlayer(PlayerItem* player)
{
   if (_start_times.contains(player))
   {
      return;
   }

   auto* material = getMaterial();
   if (!material)
   {
      return;
   }

   _start_times[player] = GlobalTime::Instance()->getTime();
   material->addMesh(player->getMesh());
}

void InvisiblePlayerEffect::removePlayer(PlayerItem* player)
{
   if (!_start_times.erase(player))
   {
      return;
   }

   player->getMesh()->setRenderParameter(1, 0.0f);

   if (auto* material = getMaterial())
   {
      material->removeMesh(player->getMesh());
   }
}

void InvisiblePlayerEffect::removeAllPlayers()
{
   while (!_start_times.empty())
   {
      removePlayer(_start_times.begin()->first);
   }
}

void InvisiblePlayerEffect::update(float global_time)
{
   for (auto it = _start_times.begin(); it != _start_times.end();)
   {
      auto* player = it->first;
      const float elapsed = global_time - it->second;

      if (elapsed >= TIME_DURATION)
      {
         ++it;
         removePlayer(player);
         continue;
      }

      float param = PARAM_MAX;
      if (elapsed < TIME_FADE_IN_END)
      {
         param = elapsed - TIME_FADE_IN_START;
      }
      else if (elapsed > TIME_FADE_OUT_START)
      {
         param = PARAM_MAX - (elapsed - TIME_FADE_OUT_START);
      }

      player->getMesh()->setRenderParameter(1, -param);
      ++it;
   }
}

void InvisiblePlayerEffect::captureBackground()
{
   if (_start_times.empty())
   {
      return;
   }

   auto* material = getMaterial();
   if (!material)
   {
      return;
   }

   if (!_background_texture)
   {
      glGenTextures(1, &_background_texture);
   }

   int32_t x = 0;
   int32_t y = 0;
   int32_t width = 0;
   int32_t height = 0;
   activeDevice->getViewPort(&x, &y, &width, &height);

   glBindTexture(GL_TEXTURE_2D, _background_texture);
   glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, width, height, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   material->setTexture(_background_texture);
}
