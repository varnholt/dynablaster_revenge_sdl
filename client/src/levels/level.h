#pragma once

#include "camerainterpolation.h"
#include "math/matrix.h"

#include <cstdint>
#include <functional>
#include <map>
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
      LevelPirate,
      LevelDummy
   };

   explicit Level(LevelType level_type);
   virtual ~Level();

   static std::string getLevelDirectoryName(LevelType level_type);
   static std::string getLevelName(LevelType level_type);

   void abort();
   bool isAborted() const;
   void load();

   //! the level's own geometry comes without its materials and textures, for the story mode: it builds the stage
   //! from the world's kit and only takes the level's cameras; set before load()
   void setArenaHidden(bool hidden);

   virtual void initialize();
   virtual void draw();

   //! draws the level's own geometry around the field
   virtual void drawLevel(float frame, const Matrix& view);
   virtual void drawBackground();
   virtual void animate(float dt);
   virtual void reset();
   virtual void loadData();

   //! key of this level's lens flare in flares.ini, empty for none
   virtual std::string getLensFlareKey() const;

   enum class Weather
   {
      Clear,
      Snow
   };

   //! weather effects the level comes with, levels without any ignore it
   virtual void setWeather(Weather weather);

   //! how far the game has come through its rounds, 0..1 from the first round's start to the last one's end;
   //! levels with a time of day go through one day and night in it
   virtual void setDayTime(float phase);

   //! the blocks' and the stones' (and their debris') skins from folder's block-unwrap.tga and stone-unwrap.tga, and
   //! the light probes the blocks, the skull, the bombs and the players are lit by from its diffuse_level.tga,
   //! specular_level.tga, specular_level2.tga (the players') and diffuse_bomb.tga, and the shadow cast onto the
   //! blocks from its shadow-cookie.tga, the level's own where the folder has none; the story mode's worlds bring
   //! their own (data/game/story/world<n>)
   void setBlockSkins(const std::string& folder);

   std::string path() const;
   SceneGraph& getScene() const;
   SceneGraph& getLevel() const;
   SceneGraph& getPlayers() const;
   Material& getKickExtra() const;
   Material& getSpeedupExtra() const;
   Material& getBombExtra() const;
   Material& getFlameExtra() const;
   Material& getSkullExtra() const;

   //! the story mode's extras and exit door, empty for levels without them
   std::optional<std::reference_wrapper<Material>> getStoryExtra(int32_t type) const;

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

   //! the light probes (sphere maps, see Material::setLightProbes()) the blocks and the debris, the players and the
   //! bombs are lit by
   void setLightProbes(
      const std::string& diffuse,
      const std::string& specular,
      const std::string& player_specular,
      const std::string& bomb_specular
   );

   //! creates _destruction and fills _destruct_anim
   void loadDestructions(Camera& shadow_camera);

   LevelType _level_type;
   std::string _block_skins;  // the folder setBlockSkins() took them from
   bool _aborted = false;
   bool _arena_hidden = false;

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
   std::map<int32_t, MaterialRef> _story_extras;
};
