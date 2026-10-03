#pragma once

#include "camerainterpolation.h"
#include "math/matrix.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Camera;
class SceneGraph;
class Material;
class Node;

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
   SceneGraph* getScene() const;
   SceneGraph* getLevel() const;
   SceneGraph* getPlayers() const;
   Material* getKickExtra() const;
   Material* getSpeedupExtra() const;
   Material* getBombExtra() const;
   Material* getFlameExtra() const;
   Material* getSkullExtra() const;

   const std::vector<Node*>& getDestructions() const;
   Material* getShadowBillboard() const;
   Material* getShadowBlockBillboard() const;
   Material* getBombMaterial() const;
   Material* getSkullMaterial() const;
   Material* getStoneMaterial() const;
   Material* getBlockMaterial() const;
   Material* getDestructionMaterial() const;

   bool isPlayerMapEmpty() const;
   void resetPlayerPositions();
   void startPositionUpdate(float width, float height, float dt);
   void addPlayerPosition(PlayerInfo* player);
   void endPlayerPositionUpdate();
   Matrix getCameraMatrix(float time, float scale = 1.0f);

protected:
   //! creates _destruction and fills _destruct_anim
   void loadDestructions(Camera* shadow_camera);

   LevelType _level_type;
   bool _aborted = false;

   // declared in reverse destruction order: level, scene, players
   std::unique_ptr<SceneGraph> _players;
   std::unique_ptr<SceneGraph> _scene;
   std::unique_ptr<SceneGraph> _level;
   std::unique_ptr<CameraInterpolation> _camera_interpolation;

   // the nodes are deleted by whoever takes getDestructions()
   std::vector<Node*> _destruct_anim;

   // materials are owned by the scene graph they were created for
   Material* _stones = nullptr;
   Material* _blocks = nullptr;
   Material* _skulls = nullptr;
   Material* _destruction = nullptr;
   Material* _extra_flame = nullptr;
   Material* _extra_bomb = nullptr;
   Material* _extra_speedup = nullptr;
   Material* _extra_kick = nullptr;
   Material* _extra_skull = nullptr;
   Material* _outlines = nullptr;
   Material* _bombs = nullptr;
   Material* _shadow_billboards = nullptr;
   Material* _shadow_blocks = nullptr;
};
