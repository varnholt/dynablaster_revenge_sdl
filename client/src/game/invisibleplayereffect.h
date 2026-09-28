#pragma once

#include <unordered_map>

class InvisibilityMaterial;
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

   void setPlayerScene(SceneGraph* players);

   void addPlayer(PlayerItem* player);
   void removePlayer(PlayerItem* player);
   void removeAllPlayers();

   void update(float global_time);

   //! snapshots the frame rendered so far - call after the level, before the players
   void captureBackground();

private:
   InvisibilityMaterial* getMaterial() const;

   SceneGraph* _scene = nullptr;
   std::unordered_map<PlayerItem*, float> _start_times;
   unsigned int _background_texture = 0;
};
