#ifndef GAMEDRAWABLE_H
#define GAMEDRAWABLE_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// tools
#include "tools/map2d.h"

// game
#include "constants.h"
#include "enemytype.h"
#include "playerboundingrect.h"

// engine
#include "drawable.h"
#include "math/matrix.h"

// shared
#include "mapitem.h"

// shared
#include "gamesignal.h"

// forward declarations
class DetonationManager;
class EnemyRenderer;
class StoryField;
class ExitPortal;
class ExtraMapItem;
class FuseParticleSystem;
class GamePlayerNameDisplay;
class Level;
class Material;
class InvisiblePlayerEffect;
class ExtraAnimations;
class LensFlareFactory;
class RibbonAnimationFactory;
class Mesh;
class MushroomAnimation;
class Node;
class PlayerDeathEffect;
class PlayerInfectedEffect;
class PlayerInvincibleEffect;
class PlayerItem;
class RenderDevice;
class SceneGraph;
class ShroomFilter;
class Skull;
class StarTalersFactory;

// Real map/players/bombs/extras/HUD rendering.
// Not implemented yet (backlog, see project memory):
// GamePlaybackDisplay, HeadlessIntegration.
// Dead code, not ported: drawTestQuad/drawBoundingBox/drawBoundingRect/playerBoundingRect/
// loadExplosion, INSPECT_SCENE debug input, mExportScene.
class GameDrawable : public Drawable
{
public:
   //! constructor
   explicit GameDrawable(RenderDevice& device);

   //! destructor
   ~GameDrawable() override;

   //! initialize gl context
   void initializeGL() override;

   //! overwrite paint
   void paintGL() override;

   //! animate scene
   void animate(float time) override;

   //! set visibility
   void setVisible(bool visible) override;

   //! getter for level path
   const std::string& getLevelPath() const;

   // event handles

   //! key release event
   void keyPressEvent(const KeyEvent& event) override;

   //! key press event
   void keyReleaseEvent(const KeyEvent& event) override;

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
   void setPlayfieldSize(int width, int height);

   void createMapItem(const MapItem& item);
   void removeMapItem(const MapItem& item);
   void destroyMapItem(const MapItem& item, float flame_count);
   void addDetonation(int x, int y, int up, int down, int left, int right, float intense);
   void loadLevel(const std::string& level);

   void addPlayer(int id, const std::string& nick, Constants::Color);
   void removePlayer(int id);
   void setPlayerPosition(int id, float x, float y, float dir);
   void setPlayerSpeed(int id, float dx, float dy, float da);
   void setPlayerId(int id);

   //! effect lab: suppress the player name tags
   void setPlayerNamesEnabled(bool enabled);
   void setMapItemPosition(int32_t item_id, float x, float y, float z);

   //! extra has been removed
   void extraRemoved(int x, int y, bool destroyed, Constants::ExtraType extra, int player_id);

   //! shake a block
   void shakeBlock(const MapItem& item);

   //! a player has been infected
   void playerInfected(int id, Constants::SkullType, int infector_id, int extra_x, int extra_y);

   //! set zoom factor of camera. default is 1.0f
   void setCameraZoom(float zoom);

   // story mode
   void addEnemy(int id, EnemyType type, float x, float y, float angle);
   void setEnemyPosition(int id, float x, float y, float angle, float dx, float dy, int flags);
   void enemyHit(int id);
   void enemyKilled(int id, bool removed);
   void setStoryStage(int stage);
   //! opens the exit once every enemy is gone, closes it again when bombing it releases new ones
   void setStoryEnemiesLeft(int enemies_left);

private:
   //! game state was changed
   void gameStateChanged();

   void playWinAnimation();

   //! what the drawable knows about a map item, by unique id
   struct TrackedItem
   {
      MapItem::ItemType _type = MapItem::Unknown;
      int32_t _x = 0;
      int32_t _y = 0;
   };

   //! getter for player
   std::optional<std::reference_wrapper<PlayerItem>> getPlayer(int id) const;

   //! getter for level dimensions
   Constants::Dimension getDimensions(float& width, float& height) const;

   std::optional<std::reference_wrapper<Mesh>> getMesh(std::optional<int32_t> item_id) const;
   Material& getExtraMaterial(Constants::ExtraType type) const;
   void removeMapItem(int32_t item_id);
   void updateNeighbouringBlocks(int item_x, int item_y);
   void addBlock(int32_t item_id, int32_t x, int32_t y);
   Mesh& createBlock(SceneGraph& scene, Material& mat, float x, float y, float scale);
   std::optional<std::reference_wrapper<Mesh>> createBomb(const MapItem& item);
   Skull& createSkull(const MapItem& item);
   Mesh& createExtra(const ExtraMapItem& extra);
   void removeBlock(int32_t x, int32_t y);
   std::optional<std::reference_wrapper<Node>>
   createDestruction(SceneGraph& scene, float x, float y, Constants::Direction direction, float flame_count);
   void shakeBoxes(float delta);
   void animateSkulls(float frame);

   void updateBlock(int item_x, int item_y);

   void resetPlayers();
   void deleteLevelData();
   void deleteMesh(Mesh& mesh);

   std::unique_ptr<Level> _level;
   std::vector<std::reference_wrapper<Node>> _destruct_anim;
   std::unique_ptr<DetonationManager> _detonations;
   std::unique_ptr<PlayerDeathEffect> _player_death_effect;
   std::unique_ptr<PlayerInfectedEffect> _player_infected_effect;
   std::unique_ptr<GamePlayerNameDisplay> _player_name_display;
   std::unique_ptr<FuseParticleSystem> _fuse_particle_system;
   std::unique_ptr<PlayerInvincibleEffect> _player_invincible_effect;
   std::unique_ptr<StarTalersFactory> _star_talers_factory;
   std::unique_ptr<MushroomAnimation> _mushroom_animation;
   std::unique_ptr<ShroomFilter> _shroom_filter;
   std::unique_ptr<InvisiblePlayerEffect> _invisible_player_effect;
   std::unique_ptr<ExtraAnimations> _extra_animations;
   std::unique_ptr<RibbonAnimationFactory> _ribbon_animation_factory;
   std::unique_ptr<LensFlareFactory> _lens_flare_factory;
   std::unique_ptr<EnemyRenderer> _enemies;
   std::unique_ptr<StoryField> _story_field;
   std::unique_ptr<ExitPortal> _exit_portal;
   int _story_enemies_left = -1;

   //! bomberman shaped enemies are drawn as players with ids from here on
   static constexpr int ENEMY_PLAYER_ID_OFFSET = 1000;
   bool _player_names_enabled = true;

   float _time = 0.0f;
   float _time_prev = 0.0f;

   // map items by unique id
   std::unordered_map<int32_t, TrackedItem> _map_items;
   std::map<int, std::unique_ptr<PlayerItem>> _player_list;
   std::unordered_map<int32_t, std::reference_wrapper<Mesh>> _meshes;
   std::unordered_map<int32_t, std::reference_wrapper<Skull>> _skull_map;
   std::unordered_map<int32_t, float> _shaking_boxes;

   std::vector<std::reference_wrapper<Node>> _destructions;
   int _player_id = -1;
   float _bounce = 0.0;
   std::string _level_path;
   float _playfield_scale_x = 1.0;
   float _playfield_scale_y = 1.0;
   // ids of the blocks and stones on the map
   Map2d<std::optional<int32_t>> _map;

   float _camera_anim = 0.0f;
   float _camera_zoom = 1.0f;

   bool _time_reset = true;

   //! win animation has been started flag
   bool _win_animation_started = false;

   //! camera follows player
   bool _camera_follows_player = true;

   //! camera shakes on detonation event;
   float _camera_shake_intensity = 1.0;
};

#endif  // GAMEDRAWABLE_H
