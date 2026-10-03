#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>

class InvisibilityMaterial;
class Mesh;
class PlayerItem;
class SceneGraph;

// fades invisible players into a refracting glass look over the background
class InvisiblePlayerEffect
{
public:
   InvisiblePlayerEffect() = default;
   ~InvisiblePlayerEffect();

   InvisiblePlayerEffect(const InvisiblePlayerEffect&) = delete;
   InvisiblePlayerEffect& operator=(const InvisiblePlayerEffect&) = delete;

   void setPlayerScene(std::optional<std::reference_wrapper<SceneGraph>> players);

   void addPlayer(const PlayerItem& player);
   void removePlayer(const PlayerItem& player);
   void removeAllPlayers();

   void update(float global_time);

   //! snapshots the frame rendered so far - call after the level, before the players
   void captureBackground();

private:
   struct InvisiblePlayer
   {
      std::reference_wrapper<Mesh> _mesh;
      float _start_time = 0.0f;
   };

   std::optional<std::reference_wrapper<InvisibilityMaterial>> getMaterial() const;
   void removePlayer(int32_t id);

   std::optional<std::reference_wrapper<SceneGraph>> _scene;
   std::map<int32_t, InvisiblePlayer> _players;
   uint32_t _background_texture = 0;
};
