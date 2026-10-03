#include "invisibleplayereffect.h"

#include "framebuffer.h"
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

void InvisiblePlayerEffect::setPlayerScene(std::optional<std::reference_wrapper<SceneGraph>> players)
{
   _scene = players;
}

std::optional<std::reference_wrapper<InvisibilityMaterial>> InvisiblePlayerEffect::getMaterial() const
{
   if (!_scene)
   {
      return std::nullopt;
   }

   if (auto* material = dynamic_cast<InvisibilityMaterial*>(&_scene->get().getMaterial(INVISIBILITY_MATERIAL_INDEX)))
   {
      return *material;
   }

   return std::nullopt;
}

void InvisiblePlayerEffect::addPlayer(const PlayerItem& player)
{
   if (_players.contains(player.getID()))
   {
      return;
   }

   const auto material = getMaterial();
   if (!material)
   {
      return;
   }

   _players.emplace(player.getID(), InvisiblePlayer{player.getMesh(), GlobalTime::Instance().getTime()});
   material->get().addMesh(player.getMesh());
}

void InvisiblePlayerEffect::removePlayer(const PlayerItem& player)
{
   removePlayer(player.getID());
}

void InvisiblePlayerEffect::removePlayer(int32_t id)
{
   const auto it = _players.find(id);
   if (it == _players.end())
   {
      return;
   }

   Mesh& mesh = it->second._mesh;
   _players.erase(it);

   mesh.setRenderParameter(1, 0.0f);

   if (const auto material = getMaterial())
   {
      material->get().removeMesh(mesh);
   }
}

void InvisiblePlayerEffect::removeAllPlayers()
{
   while (!_players.empty())
   {
      removePlayer(_players.begin()->first);
   }
}

void InvisiblePlayerEffect::update(float global_time)
{
   for (auto it = _players.begin(); it != _players.end();)
   {
      const int32_t id = it->first;
      Mesh& mesh = it->second._mesh;
      const float elapsed = global_time - it->second._start_time;

      if (elapsed >= TIME_DURATION)
      {
         ++it;
         removePlayer(id);
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

      mesh.setRenderParameter(1, -param);
      ++it;
   }
}

void InvisiblePlayerEffect::captureBackground()
{
   if (_players.empty())
   {
      return;
   }

   const auto material = getMaterial();
   if (!material)
   {
      return;
   }

   if (!_background_texture)
   {
      glGenTextures(1, &_background_texture);
   }

   const auto [x, y, width, height] = activeDevice().getViewPort();

   glBindTexture(GL_TEXTURE_2D, _background_texture);
   FrameBuffer::copyTexImage(x, y, width, height);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   material->get().setTexture(_background_texture);
}
