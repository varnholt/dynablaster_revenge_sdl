// header
#include "gamedrawable.h"

// framework
#include "framework/timerhandler.h"
#include "gldevice.h"
#include "materials/blockmaterial.h"
#include "nodes/camera.h"
#include "nodes/dummy.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"

// game
#include "animation/motionmixer.h"
#include "bombermanclient.h"
#include "detonationmanager.h"
#include "effects/lensflare/lensflarefactory.h"
#include "effects/ribbons/ribbonanimationfactory.h"
#include "extra.h"
#include "extraanimations.h"
#include "soundmanager.h"
#include "extramapitem.h"
#include "fuseparticlesystem.h"
#include "gameplayernamedisplay.h"
#include "gamesettings.h"
#include "gamestatemachine.h"
#include "invisibleplayereffect.h"
#include "levels/levelfactory.h"
#include "mapitem.h"
#include "mushroomanimation.h"
#include "playerdeatheffect.h"
#include "playerinfectedeffect.h"
#include "playerinvincibleeffect.h"
#include "playeritem.h"
#include "postproduction/shroomfilter.h"
#include "sdlglobaltime.h"
#include "skull.h"
#include "startalersfactory.h"
#include "story/enemyrenderer.h"
#include "story/exitportal.h"
#include "story/storyfield.h"

// std
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "logging.h"

#include <SDL3/SDL_keycode.h>

//-----------------------------------------------------------------------------
/*!
 */
GameDrawable::GameDrawable(RenderDevice& device) : Drawable(device)
{
   // load animations
   MotionMixer::addAnimation("player-idle.hjb");
   MotionMixer::addAnimation("player-walk.hjb");
   MotionMixer::addAnimation("player-run.hjb");
   MotionMixer::addAnimation("player-die1.hjb");
   MotionMixer::addAnimation("player-win.hjb");

   GameStateMachine::getInstance().stateChangedSignal.connect([this]() { gameStateChanged(); });
}

//-----------------------------------------------------------------------------
/*!
 */
GameDrawable::~GameDrawable()
{
   MotionMixer::cleanup();

   deleteLevelData();
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::deleteLevelData()
{
   clear();
   _destructions.clear();

   // the effects hold on to the level's player materials
   if (_player_infected_effect)
   {
      _player_infected_effect->clear();
   }

   if (_player_invincible_effect)
   {
      _player_invincible_effect->clear();
   }

   _level.reset();

   // the destructor gets here too, possibly without initializeGL() having run
   if (_invisible_player_effect)
   {
      _invisible_player_effect->setPlayerScene(std::nullopt);
   }

   if (_lens_flare_factory)
   {
      _lens_flare_factory->activate({});
   }

   _destruct_anim.clear();
}

//-----------------------------------------------------------------------------
/*!
   \param visible visible flag
*/
void GameDrawable::setVisible(bool visible)
{
   if (visible)
   {
      // reload camera settings from gameplay settings
      GameSettings::GameplaySettings& settings = GameSettings::getInstance().getGameplaySettings();

      _camera_follows_player = settings.isCameraFollowingPlayer();
      _camera_shake_intensity = settings.getCameraShakeIntensity();

      // reset
      _time_reset = true;
      _camera_anim = 0.0f;

      resetPlayers();

      if (_level)
         _level->reset();
   }
   else
   {
      _win_animation_started = false;
   }

   Drawable::setVisible(visible);
}

//-----------------------------------------------------------------------------
/*!
   \param event key press event
*/
void GameDrawable::keyPressEvent(const KeyEvent& event)
{
   if (event.key() == SDLK_TAB)
   {
      displayPlayerNames();
   }

   key_pressed_signal(event);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::displayPlayerNames()
{
   _player_name_display->start();
}

//-----------------------------------------------------------------------------
/*!
   \param event key release event
*/
void GameDrawable::keyReleaseEvent(const KeyEvent& event)
{
   key_released_signal(event);
}

//-----------------------------------------------------------------------------
/*!
 */
const std::string& GameDrawable::getLevelPath() const
{
   return _level_path;
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setPlayerId(int id)
{
   _player_id = id;
}

//-----------------------------------------------------------------------------
/*!
   \param enabled \c false never draws the player name tags (used by the effect lab)
*/
void GameDrawable::setPlayerNamesEnabled(bool enabled)
{
   _player_names_enabled = enabled;
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::clear()
{
   // removeMapItem() erases from _map_items - iterate a snapshot of the ids, not the live map.
   std::vector<int32_t> item_ids;
   item_ids.reserve(_map_items.size());
   for (const auto& [item_id, item] : _map_items)
   {
      item_ids.push_back(item_id);
   }

   for (const int32_t item_id : item_ids)
   {
      removeMapItem(item_id);
   }

   _detonations->clear();

   // the server doesn't remove the enemies still alive when a story game ends
   if (_enemies)
   {
      _enemies->clear();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::deleteMesh(Mesh& mesh)
{
   if (_level)
   {
      SceneGraph& playfield = _level->getScene();
      int count = playfield.getMaterialCount();
      for (int i = 0; i < count; i++)
      {
         playfield.getMaterial(i).removeMesh(mesh);
      }
      playfield.removeNode(mesh);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setPlayfieldScale(float scale_x, float scale_y)
{
   _playfield_scale_x = 1.0f / scale_x;
   _playfield_scale_y = 1.0f / scale_y;

   if (_level)
   {
      _level->getScene().setGlobalTransform(Matrix::scale(_playfield_scale_x, _playfield_scale_y, _playfield_scale_x));
      _level->getPlayers().setGlobalTransform(Matrix::scale(_playfield_scale_x, _playfield_scale_y, _playfield_scale_x));
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setPlayfieldSize(int width, int height)
{
   _map.init(width, height);
}

//-----------------------------------------------------------------------------
/*!
   Real LevelLoadingThread (QThread + background GL resource creation) deliberately not ported -
   see project memory, Phase 5: GL resource creation off the main thread is a real correctness
   risk this port has no infrastructure for. Loads synchronously instead, same as the already-
   proven harness castle-level demo (main_harness.cpp).
*/
void GameDrawable::loadLevel(const std::string& level_path)
{
   qDebug("GameDrawable::loadLevel: loading %s", level_path.c_str());

   _map.clear();

   resetPlayers();

   deleteLevelData();

   _level_path = "data/" + level_path;

   auto level = LevelFactory::getLevelInstance(_level_path);
   level->load();

   _level = std::move(level);

   _invisible_player_effect->setPlayerScene(_level->getPlayers());
   _lens_flare_factory->activate(_level->getLensFlareKey());

   _level->getScene().setGlobalTransform(Matrix::scale(_playfield_scale_x, _playfield_scale_y, _playfield_scale_x));

   _level->getPlayers().setGlobalTransform(Matrix::scale(_playfield_scale_x, _playfield_scale_y, _playfield_scale_x));

   _destruct_anim = _level->getDestructions();

   _level->getDestructionMaterial().clear();

   // we're done
   level_loaded_signal(_level_path);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::initializeGL()
{
   _enemies = std::make_unique<EnemyRenderer>();
   _story_field = std::make_unique<StoryField>();
   _exit_portal = std::make_unique<ExitPortal>();
   _detonations = std::make_unique<DetonationManager>();
   _detonations->init();

   _player_death_effect = std::make_unique<PlayerDeathEffect>();
   _player_infected_effect = std::make_unique<PlayerInfectedEffect>();

   _player_name_display = std::make_unique<GamePlayerNameDisplay>();
   _player_name_display->initialize();

   _fuse_particle_system = std::make_unique<FuseParticleSystem>();
   _player_invincible_effect = std::make_unique<PlayerInvincibleEffect>();

   _star_talers_factory = std::make_unique<StarTalersFactory>();
   _star_talers_factory->initialize();

   _mushroom_animation = std::make_unique<MushroomAnimation>();
   _shroom_filter = std::make_unique<ShroomFilter>();
   _shroom_filter->init();

   _invisible_player_effect = std::make_unique<InvisiblePlayerEffect>();
   _extra_animations = std::make_unique<ExtraAnimations>();
   _ribbon_animation_factory = std::make_unique<RibbonAnimationFactory>();
   _lens_flare_factory = std::make_unique<LensFlareFactory>();
}

//-----------------------------------------------------------------------------
/*!
 */
std::optional<std::reference_wrapper<Mesh>> GameDrawable::getMesh(std::optional<int32_t> item_id) const
{
   if (!item_id)
   {
      return std::nullopt;
   }

   auto it = _meshes.find(*item_id);
   if (it != _meshes.end())
      return it->second;
   else
      return std::nullopt;
}

//-----------------------------------------------------------------------------
/*!
 */
Material& GameDrawable::getExtraMaterial(Constants::ExtraType type) const
{
   switch (type)
   {
      case Constants::ExtraBomb:
         return _level->getBombExtra();
      case Constants::ExtraSpeedup:
         return _level->getSpeedupExtra();
      case Constants::ExtraKick:
         return _level->getKickExtra();
      case Constants::ExtraSkull:
         return _level->getSkullExtra();
      case Constants::ExtraFlame:
         return _level->getFlameExtra();
      default:
         if (const auto story_extra = _level->getStoryExtra(type))
         {
            return *story_extra;
         }
         return _level->getFlameExtra();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::updateBlock(int item_x, int item_y)
{
   int flags = 0;
   int bitpos = 1;

   const auto mesh = getMesh(_map.get(item_x, item_y));
   if (mesh)
   {
      for (int y = item_y - 1; y <= item_y + 1; y++)
      {
         for (int x = item_x - 1; x <= item_x + 1; x++)
         {
            if (getMesh(_map.get(x, y)))
               flags |= bitpos;
            bitpos <<= 1;
         }
      }
      mesh->get().setRenderFlags(static_cast<uint32_t>(flags));
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::updateNeighbouringBlocks(int item_x, int item_y)
{
   for (int y = item_y - 1; y <= item_y + 1; y++)
      for (int x = item_x - 1; x <= item_x + 1; x++)
         updateBlock(x, y);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::addBlock(int32_t item_id, int32_t x, int32_t y)
{
   _map.set(x, y, item_id);
   updateNeighbouringBlocks(x, y);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::removeBlock(int32_t x, int32_t y)
{
   _map.set(x, y, std::nullopt);
   updateNeighbouringBlocks(x, y);
}

//-----------------------------------------------------------------------------
/*!
 */
Mesh& GameDrawable::createBlock(SceneGraph& scene, Material& mat, float x, float y, float size)
{
   Matrix pos, scale;
   pos.identity();
   pos.translate(Vector(x + 0.5f, -y - 0.5f));
   scale = Matrix::scale(size, size, size);

   const Mesh& ref = scene.findNode<Mesh>("Block").value();
   Mesh& mesh = _level->getScene().addNode(std::make_unique<Mesh>(ref));
   mesh.setTransform(scale * pos);
   mat.addMesh(mesh);
   _level->getShadowBlockBillboard().addMesh(mesh);

   return mesh;
}

//-----------------------------------------------------------------------------
/*!
 */
Mesh& GameDrawable::createExtra(const ExtraMapItem& extra)
{
   SceneGraph& playfield = _level->getScene();
   const Mesh& extra_mesh = playfield.findNode<Mesh>("Extra").value();
   Mesh& mesh = playfield.addNode(std::make_unique<Extra>(
      extra.getExtraType(), extra_mesh.getPart(0), static_cast<float>(extra.getX()), static_cast<float>(extra.getY())
   ));
   getExtraMaterial(extra.getExtraType()).addMesh(mesh);
   _extra_animations->addReveal(extra.getX(), extra.getY());
   return mesh;
}

//-----------------------------------------------------------------------------
/*!
 */
std::optional<std::reference_wrapper<Mesh>> GameDrawable::createBomb(const MapItem& item)
{
   SceneGraph& playfield = _level->getScene();
   const auto obj = playfield.findNode<Mesh>("lunte");

   if (!obj)
   {
      return std::nullopt;
   }

   Mesh& mesh = playfield.addNode(std::make_unique<Mesh>(obj->get()));
   mesh.setAnimationFrame(_time);

   Matrix translation_matrix;
   Vector item_position = Vector(item.getX() + 0.4f, -item.getY() - 0.5f, 0.0f);
   translation_matrix.identity();
   translation_matrix.translate(item_position);

   mesh.setTransform(translation_matrix);

   _level->getBombMaterial().addMesh(mesh);
   _level->getShadowBillboard().addMesh(mesh);

   _fuse_particle_system->addEmitter(item.getUniqueId(), item_position + FuseParticleSystem::getBombOffset());

   return mesh;
}

//-----------------------------------------------------------------------------
/*!
 */
Skull& GameDrawable::createSkull(const MapItem& item)
{
   SceneGraph& playfield = _level->getScene();
   Mesh& skull_mesh = playfield.findNode<Mesh>("skull")->get();

   Skull& mesh = playfield.addNode(std::make_unique<Skull>(skull_mesh, static_cast<float>(item.getX()), static_cast<float>(item.getY())));

   mesh.setTransform(skull_mesh.getTransform() * mesh.getTransform());

   _level->getSkullMaterial().addMesh(mesh);
   _skull_map.insert_or_assign(item.getUniqueId(), mesh);

   return mesh;
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::animateSkulls(float time)
{
   // annoying 62.5 multiplier
   time *= 0.016f;

   for (const auto& [item, skull] : _skull_map)
   {
      // frame time in seconds, relative to start frame, tick-scaled - see original comment in
      // client/src/game/gamedrawable.cpp for the derivation of the 3200.0f/19200.0 constants.
      float frame = time;
      frame -= skull.get().getStartTime();
      frame *= 3200.0f;

      float f = static_cast<float>(std::fmod(static_cast<double>(frame), 19200.0));

      Mesh& ref = skull.get().getReference();
      ref.transform(f);

      Matrix transform = ref.getTransform();
      Matrix translate = skull.get().getTranslation();

      skull.get().setTransform(transform * translate);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::shakeBlock(const MapItem& item)
{
   if (_meshes.contains(item.getUniqueId()))
   {
      _shaking_boxes[item.getUniqueId()] = 1.0f;
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::playerInfected(int id, Constants::SkullType skull_type, int infector_id, int extra_x, int extra_y)
{
   const auto player = getPlayer(id);
   if (!player)
   {
      return;
   }

   PlayerItem& player_item = *player;

   if (skull_type == Constants::SkullReset)
   {
      _player_infected_effect->remove(player_item.getMaterial());
      _player_invincible_effect->remove(player_item.getMaterial());
      _invisible_player_effect->removePlayer(player_item);

      if (id == _player_id)
      {
         _mushroom_animation->abort();
      }

      return;
   }

   // the mushroom screen filter only applies to the local player
   if (skull_type == Constants::SkullMushroom && id == _player_id)
   {
      _mushroom_animation->start();
   }

   if (skull_type == Constants::SkullInvincible)
   {
      // only a picked up skull celebrates, not one passed on by an infected player
      if (infector_id == -1)
      {
         _ribbon_animation_factory->add(static_cast<float>(extra_x), static_cast<float>(extra_y));
      }

      _player_invincible_effect->add(player_item.getMaterial());
   }
   else if (skull_type == Constants::SkullInvisible)
   {
      _invisible_player_effect->addPlayer(player_item);
   }
   else
   {
      _player_infected_effect->add(player_item.getMaterial());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::createMapItem(const MapItem& item)
{
   if (!_level)
      return;

   const int32_t item_id = item.getUniqueId();

   if (!_map_items.contains(item_id))
   {
      std::optional<std::reference_wrapper<Mesh>> mesh;
      SceneGraph& playfield = _level->getScene();

      switch (item.getType())
      {
         case MapItem::Stone:
         {
            mesh = createBlock(playfield, _level->getStoneMaterial(), item.getX(), item.getY(), 0.8f);
            break;
         }

         case MapItem::Block:
         {
            mesh = createBlock(playfield, _level->getBlockMaterial(), item.getX(), item.getY(), 0.9f);
            break;
         }

         case MapItem::Bomb:
         {
            mesh = createBomb(item);
            break;
         }

         case MapItem::Extra:
         {
            const auto& extra = dynamic_cast<const ExtraMapItem&>(item);

            // skulls are tracked in _skull_map, not _meshes
            if (extra.getExtraType() == Constants::ExtraSkull)
            {
               createSkull(item);
            }
            else if (extra.getExtraType() == Constants::ExtraExit)
            {
               // the exit is a portal in the floor, not a floating extra; the blue box only
               // comes up when it opens, right away if every enemy is gone already
               _exit_portal->show(extra.getX(), extra.getY());
               if (_story_enemies_left == 0 && _exit_portal->open())
               {
                  _extra_animations->addReveal(extra.getX(), extra.getY());
               }
            }
            else
            {
               mesh = createExtra(extra);
            }

            break;
         }

         default:
            return;
      }

      _map_items[item_id] = {item.getType(), item.getX(), item.getY()};

      if (mesh)
      {
         _meshes.insert_or_assign(item_id, *mesh);

         if (item.getType() == MapItem::Stone || item.getType() == MapItem::Block)
         {
            addBlock(item_id, item.getX(), item.getY());
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::removeMapItem(const MapItem& item)
{
   removeMapItem(item.getUniqueId());
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::removeMapItem(int32_t item_id)
{
   auto it = _map_items.find(item_id);

   if (it != _map_items.end())
   {
      const TrackedItem& item = it->second;

      _shaking_boxes.erase(item_id);

      // get associated mesh
      auto m = _meshes.find(item_id);

      if (m != _meshes.end())
      {
         if (item._type == MapItem::Stone || item._type == MapItem::Block)
         {
            removeBlock(item._x, item._y);
         }

         Mesh& mesh = m->second;

         _level->getStoneMaterial().removeMesh(mesh);
         _level->getShadowBlockBillboard().removeMesh(mesh);
         _level->getBlockMaterial().removeMesh(mesh);
         _level->getBombExtra().removeMesh(mesh);
         _level->getFlameExtra().removeMesh(mesh);
         _level->getSpeedupExtra().removeMesh(mesh);
         _level->getKickExtra().removeMesh(mesh);
         _level->getBombMaterial().removeMesh(mesh);
         _level->getShadowBillboard().removeMesh(mesh);
         _fuse_particle_system->removeEmitter(item_id);
         _meshes.erase(m);
         deleteMesh(mesh);
      }
      else
      {
         auto si = _skull_map.find(item_id);

         if (si != _skull_map.end())
         {
            Skull& skull_mesh = si->second;

            _level->getSkullMaterial().removeMesh(skull_mesh);
            _skull_map.erase(si);
            deleteMesh(skull_mesh);
         }
      }

      if (item._type == MapItem::Extra && _exit_portal && _exit_portal->isAt(item._x, item._y))
      {
         _exit_portal->hide();
      }

      _map_items.erase(it);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
std::optional<std::reference_wrapper<Node>>
GameDrawable::createDestruction(SceneGraph& scene, float x, float y, Constants::Direction direction, float flame_count)
{
   // create destruction animation
   size_t template_index = 3;
   if (flame_count < 3)
      template_index = 0;
   else if (flame_count < 5)
      template_index = 1;
   else if (flame_count < 8)
      template_index = 2;

   if (template_index < _destruct_anim.size())
   {
      const Node& root = _destruct_anim[template_index];
      int rot = 0;
      switch (direction)
      {
         case Constants::DirectionUp:
            rot = 1;
            break;
         case Constants::DirectionDown:
            rot = 3;
            break;
         case Constants::DirectionLeft:
            rot = 2;
            break;
         case Constants::DirectionRight:
            rot = 0;
            break;
         default:
            break;
      }
      const Node& destruct = root.getChild(rot);

      Dummy& dummy = scene.addNode(std::make_unique<Dummy>());
      dummy.setUserTransformable(true);
      Matrix pos;
      pos = Matrix::rotateZ(rot * std::numbers::pi_v<float> * 0.5f);
      pos.translate(Vector(x + 0.5f, -y - 0.5f));
      dummy.setUserTransformable(true);
      Matrix scale = Matrix::scale(0.8f, 0.8f, 0.8f);
      dummy.setTransform(scale * pos);

      for (int i = 0; i < destruct.getChildCount(); i++)
      {
         const Node& child = destruct.getChild(i);
         if (child.id() == Node::idMesh)
         {
            const auto& ref = dynamic_cast<const Mesh&>(child);
            Mesh& mesh = scene.addNode(std::make_unique<Mesh>(ref), dummy);
            mesh.setUserTransformable(false);
            _level->getDestructionMaterial().addMesh(mesh);
            mesh.setFrame(0.0f);
         }
      }

      return dummy;
   }

   return std::nullopt;
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::destroyMapItem(const MapItem& item, float flame_count)
{
   if (_map_items.contains(item.getUniqueId()))
   {
      if (item.getType() == MapItem::Stone)
      {
         const auto dummy = createDestruction(
            _level->getScene(), static_cast<float>(item.getX()), static_cast<float>(item.getY()), item.getDestroyDirection(), flame_count
         );

         if (dummy)
            _destructions.push_back(*dummy);
      }
   }

   removeMapItem(item.getUniqueId());
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::addDetonation(int x, int y, int up, int down, int left, int right, float intense)
{
   if (_detonations)
   {
      intense = std::sqrt(intense) * 0.15f;
      if (intense > 0.5f)
         intense = 0.5f;
      if (intense > _bounce)
         _bounce = intense;
      _detonations->addDetonation(x, y, up, down, left, right);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param item item to update
   \param x x position
   \param y y position
   \param z z position
*/
void GameDrawable::setMapItemPosition(int32_t item_id, float x, float y, float z)
{
   auto iter = _meshes.find(item_id);

   if (iter != _meshes.end())
   {
      float width = 0.0f;
      float height = 0.0f;

      getDimensions(width, height);

      if (x < -ITEM_INTERPOLATION_EPS)
         x = -ITEM_INTERPOLATION_EPS;
      if (x > width + ITEM_INTERPOLATION_EPS)
         x = width + ITEM_INTERPOLATION_EPS;

      if (y < -ITEM_INTERPOLATION_EPS)
         y = -ITEM_INTERPOLATION_EPS;
      if (y > height + ITEM_INTERPOLATION_EPS)
         y = height + ITEM_INTERPOLATION_EPS;

      Mesh& mesh = iter->second;

      Matrix pos;
      pos.identity();

      pos.translate(Vector(x + 0.5f, -y - 0.5f, z));

      mesh.setTransform(pos);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param x x position where extra has been removed
   \param y y position where extra has been removed
   \param destroyed \c true if extra was destroyed
   \param player_id if of player who picked the extra up
*/
void GameDrawable::extraRemoved(int x, int y, bool destroyed, Constants::ExtraType extra, int player_id)
{
   if (destroyed)
   {
      _extra_animations->addDestroyed(x, y);
   }
   else
   {
      _star_talers_factory->add(static_cast<float>(x), static_cast<float>(y), extra);
   }

   if (player_id != -1)
   {
      if (const auto player = getPlayer(player_id))
         player->get().setFlash(1.0f);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setPlayerPosition(int id, float x, float y, float angle)
{
   float width = 1.0f;
   float height = 1.0f;

   getDimensions(width, height);

   if (const auto player = getPlayer(id))
   {
      player->get().setRotation(angle + std::numbers::pi_v<float> * 0.5f);
      player->get().setPosition(x, y);
   }
}

//-----------------------------------------------------------------------------
/*!
   set zoom factor
   \param zoom zoom factor
*/
void GameDrawable::setCameraZoom(float zoom)
{
   _camera_zoom = zoom;
}

//-----------------------------------------------------------------------------
/*!
   \param width dimension width
   \param height dimension height
   \return dimensions enum
*/
Constants::Dimension GameDrawable::getDimensions(float& width, float& height) const
{
   const auto info = BombermanClient::getInstance().getCurrentGameInformation();

   if (!info)
   {
      width = 0.0f;
      height = 0.0f;
      return Constants::DimensionInvalid;
   }

   width = static_cast<float>(info->get().getFieldWidth());
   height = static_cast<float>(info->get().getFieldHeight());

   return info->get().getMapDimensions();
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setPlayerSpeed(int id, float dx, float dy, float /*da*/)
{
   if (const auto player = getPlayer(id))
   {
      player->get().setSpeed(std::sqrt(dx * dx + dy * dy));
   }
}

//-----------------------------------------------------------------------------
/*!
 */
std::optional<std::reference_wrapper<PlayerItem>> GameDrawable::getPlayer(int id) const
{
   auto it = _player_list.find(id);
   if (it != _player_list.end())
      return *it->second;
   else
      return std::nullopt;
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::addPlayer(int id, const std::string& nick, Constants::Color color)
{
   if (const auto player = getPlayer(id))
   {
      qWarning("GameDrawable::addPlayer: player already exists");
      player->get().setKilled(false);
      return;
   }

   const auto mesh = MotionMixer::getMesh("bomberman");
   if (!mesh)
   {
      qWarning("GameDrawable::addPlayer: mesh not found");
   }

   SceneGraph& players = _level->getPlayers();
   Mesh& p = players.addNode(std::make_unique<Mesh>());
   p.copy(mesh->get());
   p.setMotionMixer(std::make_unique<MotionMixer>());
   p.setVisible(true);

   auto player = std::make_unique<PlayerItem>(id, nick, color, p);
   Material& player_material = players.getMaterial(static_cast<int32_t>(color - 1));
   player_material.addMesh(p);
   player->setMaterial(player_material);
   _player_list[id] = std::move(player);

   _level->getShadowBillboard().addMesh(p);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::removePlayer(int id)
{
   if (const auto player = getPlayer(id))
   {
      player->get().kill();
      _player_infected_effect->remove(player->get().getMaterial());
      _invisible_player_effect->removePlayer(*player);
   }

   if (id == _player_id && _mushroom_animation->isActive())
   {
      _mushroom_animation->abort();
   }

   // check for survivors, the story mode's enemies don't count
   if (_player_list.size() > 1 && !BombermanClient::getInstance().isStory())
   {
      int alive = 0;
      for (const auto& [player_id, p] : _player_list)
      {
         if (!p->isKilled())
            alive++;
      }

      if (alive == 1)
      {
         for (const auto& [player_id, p] : _player_list)
         {
            if (!p->isKilled())
            {
               if (!_win_animation_started)
               {
                  _win_animation_started = true;
                  TimerHandler::singleShot(2000, [this]() { playWinAnimation(); });
               }
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::playWinAnimation()
{
   for (const auto& [player_id, player] : _player_list)
   {
      if (!player->isKilled())
      {
         player->win();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::gameStateChanged()
{
   switch (GameStateMachine::getInstance().getState())
   {
      case Constants::GameStopped:
         _player_infected_effect->clear();
         _win_animation_started = false;
         break;

      default:
         break;
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::animate(float time)
{
   if (_detonations)
      _detonations->update(time / 62.5f);

   if (_time_reset)
   {
      _time = time;
      _time_reset = false;
   }

   float delta = time - _time;
   _time = time;

   if (_player_death_effect)
      _player_death_effect->animate(delta);

   _player_infected_effect->animate(delta);

   _fuse_particle_system->animate(delta);
   _player_invincible_effect->animate(delta);
   _star_talers_factory->update(delta * 0.05f);

   _camera_anim += delta * 60.0f;

   if (_bounce > delta * 0.01f)
      _bounce -= delta * 0.01f;
   else
      _bounce = 0.0f;

   // rotational rotation is rotating:
   for (const auto& [item_id, item_mesh] : _meshes)
   {
      switch (_map_items.at(item_id)._type)
      {
         case MapItem::Extra:
         {
            dynamic_cast<Extra&>(item_mesh.get()).animate(time);
         }
         break;

         case MapItem::Bomb:
         {
            Mesh& mesh = item_mesh;
            float t = time * 0.1f + mesh.getAnimationFrame();

            Vector pos = mesh.getTransform().translation();

            _fuse_particle_system->setEmitterPosition(item_id, pos + FuseParticleSystem::getBombOffset());

            float sx = 1.0f + std::sin(t) * 0.2f;
            float sy = 1.0f - std::sin(t) * 0.3f;

            Matrix mat = Matrix::scale(sx, sx, sy);
            mat.translate(pos);
            mesh.setTransform(mat);
         }
         break;

         default:
            break;
      }
   }

   for (const auto& [player_id, player] : _player_list)
   {
      player->animate(time, delta);
   }

   // update destructions and remove if end of animation was reached
   for (auto it = _destructions.begin(); it != _destructions.end();)
   {
      Node& destr = *it;
      bool remove = false;
      for (int i = 0; i < destr.getChildCount(); i++)
      {
         Mesh& mesh = dynamic_cast<Mesh&>(destr.getChild(i));
         float frame = mesh.getFrame() + delta * 30.0f;
         if (frame > 4000)
            remove = true;
         mesh.setFrame(frame);
      }

      if (remove)
      {
         it = _destructions.erase(it);
         // deleteMesh() unlinks the mesh from destr, so walk the children from the back
         for (int i = destr.getChildCount() - 1; i >= 0; i--)
         {
            Mesh& mesh = dynamic_cast<Mesh&>(destr.getChild(i));
            _level->getDestructionMaterial().removeMesh(mesh);
            deleteMesh(mesh);
         }
         _level->getScene().removeNode(destr);
      }
      else
         it++;
   }

   if (_level)
      _level->animate(delta);

   if (_enemies)
      _enemies->animate(delta * 0.016f);

   animateSkulls(time);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::addEnemy(int id, EnemyType type, float x, float y, float angle)
{
   if (EnemyRenderer::isBomberman(type))
   {
      // black bomberman and his henchmen look like players
      static constexpr std::array<Constants::Color, 4> henchmen{
         Constants::ColorRed, Constants::ColorBlue, Constants::ColorGreen, Constants::ColorYellow
      };
      const Constants::Color color = type == EnemyType::BlackBomberman ? Constants::ColorBlack : henchmen[static_cast<size_t>(id) % 4];
      addPlayer(id + ENEMY_PLAYER_ID_OFFSET, "", color);
      setPlayerPosition(id + ENEMY_PLAYER_ID_OFFSET, x, y, angle);
      return;
   }

   _enemies->add(id, type, x, y, angle);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setEnemyPosition(int id, float x, float y, float angle, float dx, float dy, int flags)
{
   if (getPlayer(id + ENEMY_PLAYER_ID_OFFSET))
   {
      setPlayerPosition(id + ENEMY_PLAYER_ID_OFFSET, x, y, angle);
      setPlayerSpeed(id + ENEMY_PLAYER_ID_OFFSET, dx, dy, 0.0f);
      return;
   }

   _enemies->setPosition(id, x, y, angle, dx, dy, flags);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::enemyHit(int id)
{
   if (const auto player = getPlayer(id + ENEMY_PLAYER_ID_OFFSET))
   {
      player->get().setFlash(1.0f);
      return;
   }

   _enemies->hit(id);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::enemyKilled(int id, bool removed)
{
   if (getPlayer(id + ENEMY_PLAYER_ID_OFFSET))
   {
      removePlayer(id + ENEMY_PLAYER_ID_OFFSET);
      return;
   }

   _enemies->kill(id, removed);
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::setStoryStage(int stage)
{
   // a new stage starts in its top left corner, no scrolling there from the last one
   if (stage != _story_stage)
   {
      _story_stage = stage;
      _story_field->snap();
   }

   _story_field->setWorld(stage / 8 + 1);

   if (_level)
   {
      _level->setWeather(_story_field->getWeather());
   }
}

void GameDrawable::setStoryEnemiesLeft(int enemies_left)
{
   _story_enemies_left = enemies_left;

   if (enemies_left == 0 && _exit_portal->open())
   {
      _extra_animations->addReveal(_exit_portal->getX(), _exit_portal->getY());
      SoundManager::getInstance().playSoundExtraRevealed();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::shakeBoxes(float delta)
{
   for (auto it = _shaking_boxes.begin(); it != _shaking_boxes.end();)
   {
      float time = it->second;
      time -= delta;
      if (time < 0.0f)
         time = 0.0f;
      float intense = time * time;
      const auto mesh = getMesh(it->first);
      if (mesh)
      {
         const Matrix& cur = mesh->get().getTransform();

         Matrix scale;
         float x = 0.8f + std::sin((1.0f - time) * 14.0f) * 0.1f * intense;
         float y = 0.8f - std::sin((1.0f - time) * 12.0f) * 0.1f * intense;
         float z = 0.6f + std::cos((1.0f - time) * 16.0f) * 0.2f * intense + 0.2f * (1.0f - intense);

         scale = Matrix::scale(x, y, z);
         scale.translate(cur.translation());
         mesh->get().setTransform(scale);
      }

      if (!mesh || time <= 0.0f)
         it = _shaking_boxes.erase(it);
      else
      {
         it->second = time;
         it++;
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   Renders straight to the bound framebuffer; the shroom filter snapshots it via glCopyTexImage2D.
*/
void GameDrawable::paintGL()
{
   float time = GlobalTime::Instance().getTime();

   _invisible_player_effect->update(time);

   float dt = _time - _time_prev;

   shakeBoxes(dt * 0.015f);

   float width, height;
   Constants::Dimension dimensions;
   dimensions = getDimensions(width, height);

   float bounce_x, bounce_y;

   bounce_x = _camera_shake_intensity * 0.5f * std::sin(time * 71.0f) * _bounce;
   bounce_y = _camera_shake_intensity * 0.5f * std::sin(time * 113.0f) * _bounce;

   if (dimensions == Constants::Dimension19x17)
   {
      bounce_x *= 0.66f;
      bounce_y *= 0.66f;
   }

   Matrix view;

   const bool story = BombermanClient::getInstance().isStory();

   if (_level && story)
   {
      // one screen of the field is framed like the arena, it scrolls over larger stages
      const BombermanClient& client = BombermanClient::getInstance();
      _story_field->setSize(static_cast<int>(width), static_cast<int>(height));

      if (const auto local = client.getPlayerInfoMap().find(_player_id); local != client.getPlayerInfoMap().end())
      {
         _story_field->follow(local->second.getX(), local->second.getY(), dt * 0.016f);
      }

      const float offset_x = _story_field->getOffsetX();
      const float offset_y = _story_field->getOffsetY();

      _level->startPositionUpdate(StoryField::SCREEN_WIDTH, StoryField::SCREEN_HEIGHT, dt);
      for (const auto& [player_id, player] : client.getPlayerInfoMap())
      {
         if (client.isLocalPlayer(player_id))
         {
            PlayerInfo shifted;
            shifted.setId(player.getId());
            shifted.setKilled(player.isKilled());
            shifted.setPosition(player.getX() - offset_x, player.getY() - offset_y, player.getAngle());
            _level->addPlayerPosition(shifted);
         }
      }
      _level->endPlayerPositionUpdate();

      view = Matrix::position(-offset_x, offset_y, 0.0f) * _level->getCameraMatrix(_camera_anim, _camera_zoom);
   }
   else if (_level)
   {
      _level->startPositionUpdate(width, height, dt);

      if (_camera_follows_player)
      {
         // keep every player on this machine in view
         const BombermanClient& client = BombermanClient::getInstance();
         for (const auto& [player_id, player] : client.getPlayerInfoMap())
         {
            if (client.isLocalPlayer(player_id))
            {
               _level->addPlayerPosition(player);
            }
         }

         // if no players have been added to the camera interpolation; then add all players
         if (_level->isPlayerMapEmpty())
         {
            for (const auto& [player_id, p] : client.getPlayerInfoMap())
               _level->addPlayerPosition(p);
         }
      }

      _level->endPlayerPositionUpdate();

      view = _level->getCameraMatrix(_camera_anim, _camera_zoom);
   }

   Matrix shake = Matrix::position(bounce_x, bounce_y, 0.0f) * view;

   // clears the frame, space draws its starfield and earth here
   if (_level)
   {
      _level->drawBackground();
   }

   // render scene
   if (_level)
   {
      if (story)
      {
         // the stage is built from the world's kit instead of the arena, with the arena's camera
         _level->getScene().setupCamera(shake);
         _story_field->render();
      }
      else
      {
         _level->getLevel().render(_camera_anim, shake);
      }

      _exit_portal->render(dt);

      _level->getScene().render(0.0, shake);
      _invisible_player_effect->captureBackground();
      _level->getPlayers().render(0.0, shake);

      if (story)
      {
         _enemies->render();
      }
   }

   _detonations->render();
   _fuse_particle_system->render();
   _player_invincible_effect->render();
   _star_talers_factory->render();

   // start flow fields when a player got killed (and the kill anim is over) - matches the
   // original's own "player_mesh->getFrame() > 10000.0f" convention: PlayerItem::animate() only
   // wraps the frame counter back to 0 while alive, so it climbs unbounded once _killed is set,
   // naturally crossing 10000 once the one-shot death animation has long finished playing.
   for (const auto& [player_id, player] : _player_list)
   {
      if (player->isKilled())
      {
         Mesh& player_mesh = player->getMesh();

         if (player_mesh.getFrame() > 10000.0f)
         {
            Geometry& player_geometry = player_mesh.getPart(0);

            if (player_geometry.isVisible())
            {
               Material& material = _level->getPlayers().getMaterial(static_cast<int32_t>(player->getColor()) - 1);
               _player_death_effect->add(material);
               player_geometry.setVisible(false);
            }
         }
      }
   }

   _player_death_effect->render();
   _player_infected_effect->render();
   _extra_animations->render(dt);
   _ribbon_animation_factory->update(dt);

   // draw player names
   if (_player_names_enabled && _player_name_display->isActive())
   {
      _player_name_display->setPlayerData(_player_list);
      _player_name_display->draw();
   }

   // draw level specific stuff
   if (_level)
      _level->draw();

   if (_mushroom_animation->isActive())
   {
      _mushroom_animation->update();
      _shroom_filter->setIntensity(_mushroom_animation->getIntensity());
      _shroom_filter->setTime(GlobalTime::Instance().getTime());
      _shroom_filter->apply();
   }

   _lens_flare_factory->draw();

   _time_prev = _time;
}

//-----------------------------------------------------------------------------
/*!
 */
void GameDrawable::resetPlayers()
{
   if (_level)
      _level->resetPlayerPositions();

   if (_invisible_player_effect)
   {
      _invisible_player_effect->removeAllPlayers();
   }

   auto it = _player_list.begin();
   while (it != _player_list.end())
   {
      const std::unique_ptr<PlayerItem> player = std::move(it->second);
      it = _player_list.erase(it);

      int color = static_cast<int32_t>(player->getColor());
      Mesh& mesh = player->getMesh();
      SceneGraph& players = _level->getPlayers();
      Material& player_material = players.getMaterial(static_cast<int32_t>(color - 1));
      player_material.removeMesh(mesh);
      _level->getShadowBillboard().removeMesh(mesh);
      players.removeNode(mesh);
   }
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if camera follows player
*/
bool GameDrawable::isCameraFollowingPlayer() const
{
   return _camera_follows_player;
}

//-----------------------------------------------------------------------------
/*!
   \param value camera follows player flag
*/
void GameDrawable::setCameraFollowingPlayer(bool value)
{
   _camera_follows_player = value;
}
