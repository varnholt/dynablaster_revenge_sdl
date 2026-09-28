#ifndef GAMEDRAWABLE_H
#define GAMEDRAWABLE_H

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// tools
#include "tools/map2d.h"
#include "tools/array.h"

// game
#include "constants.h"
#include "playerboundingrect.h"

// engine
#include "drawable.h"
#include "math/matrix.h"

// shared
#include "signal.h"

// forward declarations
class DetonationManager;
class ExtraMapItem;
class FuseParticleSystem;
class GamePlayerNameDisplay;
class Level;
class MapItem;
class Material;
class Mesh;
class Node;
class PlayerDeathEffect;
class PlayerInfectedEffect;
class PlayerInvincibleEffect;
class PlayerItem;
class RenderDevice;
class SceneGraph;
class Skull;
class StarTalersFactory;

// Real map/players/bombs/extras/HUD rendering.
// Not implemented yet (backlog, see project memory): ExtraAnimation/ExtraRevealAnimation,
// MushroomAnimation+ShroomFilter, InvisiblePlayerEffect,
// RibbonAnimationFactory/LensFlareFactory, GamePlaybackDisplay, HeadlessIntegration.
// Dead code, not ported: drawTestQuad/drawBoundingBox/drawBoundingRect/playerBoundingRect/
// loadExplosion, INSPECT_SCENE debug input, mExportScene.
class GameDrawable : public Drawable
{
public:

   //! constructor
   GameDrawable(RenderDevice*);

   //! destructor
   virtual ~GameDrawable();

   //! initialize gl context
   void initializeGL();

   //! overwrite paint
   void paintGL();

   //! animate scene
   void animate(float time);

   //! set visibility
   void setVisible(bool visible);

   //! getter for level path
   const std::string& getLevelPath() const;

   // event handles

   //! key release event
   void keyPressEvent(const KeyEvent& event);

   //! key press event
   void keyReleaseEvent(const KeyEvent& event);

   //! key press/release notification
   Signal<const KeyEvent&> key_pressed_signal;
   Signal<const KeyEvent&> key_released_signal;

   //! level finished loading
   Signal<const std::string&> level_loaded_signal;

   //! show player-name overlay + own-position arrow (Tab key)
   void displayPlayerNames();

   //! getter for camera following player
   bool isCameraFollowingPlayer() const;

   //! setter for camera following player
   void setCameraFollowingPlayer(bool value);


public:

   void clear();

   void setPlayfieldScale(float x_scale = 1.0, float y_scale = 1.0f);
   void setPlayfieldSize(int width,int height);

   void createMapItem(MapItem *item);
   void removeMapItem(MapItem *item);
   void destroyMapItem(MapItem *item, float flame_count);
   void addDetonation(int x, int y, int up, int down, int left, int right, float intense);
   void loadLevel(const std::string& level);

   void addPlayer(int id, const std::string& nick, Constants::Color);
   void removePlayer(int id);
   void setPlayerPosition(int id, float x, float y, float dir);
   void setPlayerSpeed(int id, float dx, float dy, float da);
   void setPlayerId(int id);
   void setMapItemPosition(MapItem*, float x, float y, float z);

   //! extra has been removed
   void extraRemoved(
      int x,
      int y,
      bool destroyed,
      Constants::ExtraType extra,
      int player_id
   );

   //! shake a block
   void shakeBlock(MapItem* item);

   //! a player has been infected
   void playerInfected(
      int id,
      Constants::SkullType,
      int infector_id,
      int extra_x,
      int extra_y
   );

   //! set zoom factor of camera. default is 1.0f
   void setCameraZoom(float zoom);

private:

   //! game state was changed
   void gameStateChanged();

   void playWinAnimation();

   //! getter for player
   PlayerItem* getPlayer(int id) const;

   //! getter for level dimensions
   Constants::Dimension getDimensions(
      float& width,
      float& height
   ) const;

   Mesh* getMesh(MapItem *item) const;
   Mesh* getSkullMesh(MapItem* item) const;
   void updateNeighbouringBlocks(int item_x, int item_y);
   void addBlock(MapItem *item);
   Mesh* createBlock(SceneGraph* scene, Material* mat, float x, float y, float scale);
   Mesh* createBomb(MapItem *item);
   Mesh* createSkull(MapItem* item);
   Mesh* createExtra(ExtraMapItem *extra);
   void removeBlock(MapItem *item);
   Node* createDestruction(SceneGraph *scene, float x, float y, Constants::Direction direction, float flame_count);
   void shakeBoxes(float delta);
   void animateSkulls(float frame);

   void updateBlock(int item_x, int item_y);

   void resetPlayers();
   void deleteLevelData();
   void deleteMesh(Mesh *mesh);

   Level* _level;
   SceneGraph* _playfield;
   SceneGraph* _level_scene_graph;
   SceneGraph* _players;
   Array<Node*> _destruct_anim;
   DetonationManager* _detonations;
   PlayerDeathEffect* _player_death_effect;
   PlayerInfectedEffect* _player_infected_effect;
   GamePlayerNameDisplay* _player_name_display;
   FuseParticleSystem* _fuse_particle_system;
   PlayerInvincibleEffect* _player_invincible_effect;
   StarTalersFactory* _star_talers_factory;

   float _time;
   float _time_prev;

   std::unordered_set<MapItem*> _map_items;
   std::vector<MapItem*> _stone_list;
   std::map<int,PlayerItem*> _player_list;
   std::unordered_map<MapItem*,Mesh*> _meshes;
   std::unordered_map<MapItem*,Skull*> _skull_map;
   std::unordered_map<int,Material*> _extra_materials;
   std::unordered_map<MapItem*,float> _shaking_boxes;

   Material *_stones;
   Material *_blocks;
   Material *_skulls;
   Material *_destruction;
   Material *_extra_flame;
   Material *_extra_bomb;
   Material *_extra_speedup;
   Material *_extra_kick;
   Material *_extra_skull;
   Material *_bombs;
   Material *_shadow_billboards;
   Material *_shadow_blocks;

   std::vector<Node*> _destructions;
   int     _player_id;
   float   _bounce;
   std::string _level_path;
   float   _playfield_scale_x;
   float   _playfield_scale_y;
   Map2d<MapItem> _map;

   float _camera_anim;
   float _camera_zoom;

   bool _time_reset;

   //! win animation has been started flag
   bool _win_animation_started;

   //! camera follows player
   bool _camera_follows_player;

   //! camera shakes on detonation event;
   float _camera_shake_intensity;
};

#endif // GAMEDRAWABLE_H
