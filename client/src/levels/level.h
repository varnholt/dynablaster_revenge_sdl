#pragma once

#include "camerainterpolation.h"
#include "math/matrix.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Camera;
class SceneGraph;
class Material;
class Node;
class PlayerInfo;

constexpr int32_t MAX_PLAYERS = 10;

class Level
{
public:
   enum LevelType
   {
      LevelCastle,
      LevelMansion,
      LevelSpace,
      LevelDummy
   };

   explicit Level(LevelType level_type);
   virtual ~Level();

   static std::string getLevelDirectoryName(LevelType level_type);
   static std::string getLevelName(LevelType level_type);

   void abort();
   bool isAborted() const;
   void load();

   virtual void initialize();
   virtual void draw();
   virtual void drawBackground();
   virtual void animate(float dt);
   virtual void reset();
   virtual void loadData();

   //! key of this level's lens flare in flares.ini, empty for none
   virtual std::string getLensFlareKey() const;

   std::string path() const;
   SceneGraph& getScene() const;
   SceneGraph& getLevel() const;
   SceneGraph& getPlayers() const;
   Material& getKickExtra() const;
   Material& getSpeedupExtra() const;
   Material& getBombExtra() const;
   Material& getFlameExtra() const;
   Material& getSkullExtra() const;

   // the four destruction animations, owned by the level
   const std::vector<std::reference_wrapper<Node>>& getDestructions() const;
   Material& getShadowBillboard() const;
   Material& getShadowBlockBillboard() const;
   Material& getBombMaterial() const;
   Material& getSkullMaterial() const;
   Material& getStoneMaterial() const;
   Material& getBlockMaterial() const;
   Material& getDestructionMaterial() const;

   bool isPlayerMapEmpty() const;
   void resetPlayerPositions();
   void startPositionUpdate(float width, float height, float dt);
   void addPlayerPosition(const PlayerInfo& player);
   void endPlayerPositionUpdate();
   Matrix getCameraMatrix(float time, float scale = 1.0f);

protected:
   using MaterialRef = std::optional<std::reference_wrapper<Material>>;

   //! creates _destruction and fills _destruct_anim
   void loadDestructions(Camera& shadow_camera);

   LevelType _level_type;
   bool _aborted = false;

   // declared in reverse destruction order: level, scene, players, destruction templates, their sources
   std::vector<std::unique_ptr<SceneGraph>> _destruction_sources;  // the template meshes keep their skeletons
   std::unique_ptr<SceneGraph> _destruction_templates;
   std::unique_ptr<SceneGraph> _players;
   std::unique_ptr<SceneGraph> _scene;
   std::unique_ptr<SceneGraph> _level;
   std::unique_ptr<CameraInterpolation> _camera_interpolation;

   // root nodes of the destruction templates
   std::vector<std::reference_wrapper<Node>> _destruct_anim;

   // materials are owned by the scene graph they were created for
   MaterialRef _stones;
   MaterialRef _blocks;
   MaterialRef _skulls;
   MaterialRef _destruction;
   MaterialRef _extra_flame;
   MaterialRef _extra_bomb;
   MaterialRef _extra_speedup;
   MaterialRef _extra_kick;
   MaterialRef _extra_skull;
   MaterialRef _bombs;
   MaterialRef _shadow_billboards;
   MaterialRef _shadow_blocks;
};
