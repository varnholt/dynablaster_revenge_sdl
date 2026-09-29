// header
#include "gamedrawable.h"

// framework
#include "gldevice.h"
#include "framework/timerhandler.h"
#include "nodes/camera.h"
#include "nodes/dummy.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "materials/blockmaterial.h"
#include "tools/filestream.h"

// game
#include "animation/motionmixer.h"
#include "bombermanclient.h"
#include "detonationmanager.h"
#include "fuseparticlesystem.h"
#include "mushroomanimation.h"
#include "gameplayernamedisplay.h"
#include "invisibleplayereffect.h"
#include "startalersfactory.h"
#include "playerdeatheffect.h"
#include "playerinfectedeffect.h"
#include "playerinvincibleeffect.h"
#include "extra.h"
#include "extraanimations.h"
#include "effects/lensflare/lensflarefactory.h"
#include "effects/ribbons/ribbonanimationfactory.h"
#include "extramapitem.h"
#include "gamesettings.h"
#include "gamestatemachine.h"
#include "levels/levelfactory.h"
#include "mapitem.h"
#include "playeritem.h"
#include "skull.h"
#include "postproduction/shroomfilter.h"
#include "sdlglobaltime.h"

// std
#include <cmath>
#include <cstdint>
#include <numbers>

#include "logging.h"

#include <SDL3/SDL_keycode.h>


//-----------------------------------------------------------------------------
/*!
*/
GameDrawable::GameDrawable(RenderDevice* device) : Drawable(device)
{
   // load animations
   MotionMixer::addAnimation("player-idle.hjb");
   MotionMixer::addAnimation("player-walk.hjb");
   MotionMixer::addAnimation("player-run.hjb");
   MotionMixer::addAnimation("player-die1.hjb");
   MotionMixer::addAnimation("player-win.hjb");

   GameStateMachine::getInstance()->stateChangedSignal.connect([this]() { gameStateChanged(); });
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

   _level.reset();

   _playfield= nullptr;
   _level_scene_graph= nullptr;
   _players= nullptr;

   // the destructor gets here too, possibly without initializeGL() having run
   if (_invisible_player_effect)
   {
      _invisible_player_effect->setPlayerScene(nullptr);
   }

   if (_lens_flare_factory)
   {
      _lens_flare_factory->activate({});
   }

   for (int i=0;i<_destruct_anim.size(); i++)
      delete _destruct_anim[i];
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
      GameSettings::GameplaySettings* settings =
         GameSettings::getInstance()->getGameplaySettings();

      _camera_follows_player = settings->isCameraFollowingPlayer();
      _camera_shake_intensity = settings->getCameraShakeIntensity();

      // reset
      _time_reset = true;
      _camera_anim= 0.0f;

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
const std::string &GameDrawable::getLevelPath() const
{
   return _level_path;
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::setPlayerId(int id)
{
   _player_id= id;
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
   // removeMapItem() erases from _map_items - iterate a snapshot copy, not the live set.
   const std::unordered_set<MapItem*> items = _map_items;
   for (MapItem* item : items)
      removeMapItem(item);

   _detonations->clear();
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::deleteMesh(Mesh *mesh)
{
   if (_playfield)
   {
      int count= _playfield->getMaterialCount();
      for (int i=0; i<count; i++)
      {
         Material* mat= _playfield->getMaterial(i);
         if (mat)
            mat->removeMesh(mesh);
      }
   }
   delete mesh;
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::setPlayfieldScale(float scale_x, float scale_y)
{
   _playfield_scale_x = 1.0f / scale_x;
   _playfield_scale_y = 1.0f / scale_y;

   if (_playfield)
   {
      _playfield->setGlobalTransform(
         Matrix::scale(
            _playfield_scale_x,
            _playfield_scale_y,
            _playfield_scale_x
         )
      );
   }

   if (_players)
   {
      _players->setGlobalTransform(
         Matrix::scale(
            _playfield_scale_x,
            _playfield_scale_y,
            _playfield_scale_x
         )
      );
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::setPlayfieldSize(int width, int height)
{
   _map.init( width, height );
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

   _level_scene_graph= _level->getLevel();
   _playfield= _level->getScene();
   _players= _level->getPlayers();
   _invisible_player_effect->setPlayerScene(_players);
   _lens_flare_factory->activate(_level->getLensFlareKey());

   _playfield->setGlobalTransform(
      Matrix::scale(
         _playfield_scale_x,
         _playfield_scale_y,
         _playfield_scale_x
      )
   );

   _players->setGlobalTransform(
      Matrix::scale(
         _playfield_scale_x,
         _playfield_scale_y,
         _playfield_scale_x
      )
   );

   _extra_flame   = _level->getFlameExtra();
   _extra_bomb    = _level->getBombExtra();
   _extra_speedup = _level->getSpeedupExtra();
   _extra_kick    = _level->getKickExtra();
   _extra_skull   = _level->getSkullExtra();

   _extra_materials[Constants::ExtraFlame]   = _extra_flame;
   _extra_materials[Constants::ExtraBomb]    = _extra_bomb;
   _extra_materials[Constants::ExtraSpeedup] = _extra_speedup;
   _extra_materials[Constants::ExtraKick]    = _extra_kick;
   _extra_materials[Constants::ExtraSkull]   = _extra_skull;

   _destruct_anim= _level->getDestructions();

   _shadow_billboards= _level->getShadowBillboard();
   _shadow_blocks= _level->getShadowBlockBillboard();
   _bombs= _level->getBombMaterial();
   _stones= _level->getStoneMaterial();
   _blocks= _level->getBlockMaterial();
   _skulls= _level->getSkullMaterial();

   _destruction= _level->getDestructionMaterial();
   _destruction->clear();

   // we're done
   level_loaded_signal(_level_path);
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::initializeGL()
{
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
Mesh* GameDrawable::getMesh(MapItem* item) const
{
   auto it= _meshes.find(item);
   if (it != _meshes.end())
      return it->second;
   else
      return nullptr;
}


//-----------------------------------------------------------------------------
/*!
*/
Mesh* GameDrawable::getSkullMesh(MapItem* item) const
{
   auto it= _skull_map.find(item);
   if (it != _skull_map.end())
      return it->second;
   else
      return nullptr;
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::updateBlock(int item_x, int item_y)
{
   int flags= 0;
   int bitpos= 1;

   MapItem* item= _map.get(item_x, item_y);
   Mesh* mesh= getMesh( item );
   if (mesh)
   {
      for (int y=item_y-1; y<=item_y+1; y++)
      {
         for (int x=item_x-1; x<=item_x+1; x++)
         {
            MapItem* item= _map.get(x,y);
            if ( getMesh(item) )
               flags |= bitpos;
            bitpos<<=1;
         }
      }
      mesh->setRenderFlags(static_cast<uint32_t>(flags));
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::updateNeighbouringBlocks(int item_x, int item_y)
{
   for (int y=item_y-1; y<=item_y+1; y++)
      for (int x=item_x-1; x<=item_x+1; x++)
         updateBlock(x, y);
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::addBlock(MapItem *item)
{
   _map.set(item->getX(), item->getY(), item);
   updateNeighbouringBlocks(item->getX(), item->getY());
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::removeBlock(MapItem *item)
{
   _map.set(item->getX(), item->getY(), nullptr);
   updateNeighbouringBlocks(item->getX(), item->getY());
}


//-----------------------------------------------------------------------------
/*!
*/
Mesh* GameDrawable::createBlock(SceneGraph* scene, Material* mat, float x, float y, float size)
{
   Matrix pos,scale;
   pos.identity();
   pos.translate( Vector(x+0.5f, -y-0.5f) );
   scale= Matrix::scale(size, size, size);

   Mesh *ref= dynamic_cast<Mesh*>(scene->getNode("Block"));
   Mesh *mesh= new Mesh(*ref);
   mesh->setTransform(scale * pos);
   _playfield->addNode(mesh);
   mat->addMesh(mesh);
   _shadow_blocks->addMesh(mesh);

   return mesh;
}


//-----------------------------------------------------------------------------
/*!
*/
Mesh* GameDrawable::createExtra(ExtraMapItem *extra)
{
   Mesh* mesh = nullptr;
   Mesh *extra_mesh= dynamic_cast<Mesh*>(_playfield->getNode("Extra"));
   mesh= new Extra(extra->getExtraType(), extra_mesh->getPart(0), extra->getX(), extra->getY());
   _extra_materials[extra->getExtraType()]->addMesh(mesh);
   _extra_animations->addReveal(extra->getX(), extra->getY());
   return mesh;
}


//-----------------------------------------------------------------------------
/*!
*/
Mesh* GameDrawable::createBomb(MapItem *item)
{
   Mesh* mesh = nullptr;

   Mesh *obj= dynamic_cast<Mesh*>(_playfield->getNode("lunte"));

   if (obj)
   {
      mesh= new Mesh(*obj);
      mesh->setAnimationFrame(_time);

      Matrix translation_matrix;
      Vector item_position =
         Vector(item->getX()+0.4f, -item->getY()-0.5f, 0.0f);
      translation_matrix.identity();
      translation_matrix.translate( item_position );

      mesh->setTransform(translation_matrix);

      _playfield->addNode(mesh);
      _bombs->addMesh(mesh);
      _shadow_billboards->addMesh(mesh);

      _fuse_particle_system->addEmitter(item, item_position + FuseParticleSystem::getBombOffset());
   }

   return mesh;
}


//-----------------------------------------------------------------------------
/*!
*/
Mesh *GameDrawable::createSkull(MapItem *item)
{
   Mesh* skull_mesh = dynamic_cast<Mesh*>(_playfield->getNode("skull"));

   Skull* mesh = new Skull(skull_mesh, item->getX(), item->getY());

   mesh->setTransform(skull_mesh->getTransform() * mesh->getTransform());

   _playfield->addNode(mesh);
   _skulls->addMesh(mesh);
   _skull_map[item] = mesh;

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
      frame -= skull->getStartTime();
      frame *= 3200.0f;

      float f = static_cast<float>(std::fmod(static_cast<double>(frame), 19200.0));

      Mesh* ref = skull->getReference();
      ref->transform(f);

      Matrix transform = ref->getTransform();
      Matrix translate = skull->getTranslation();

      skull->setTransform(transform * translate);
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::shakeBlock(MapItem* item)
{
   auto it= _meshes.find( item );
   if (it != _meshes.end())
   {
      _shaking_boxes[item] = 1.0f;
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::playerInfected(
   int id,
   Constants::SkullType skull_type,
   int infector_id,
   int extra_x,
   int extra_y
)
{
   PlayerItem* player_item = getPlayer(id);
   if (!player_item)
   {
      return;
   }

   if (skull_type == Constants::SkullReset)
   {
      _player_infected_effect->remove(player_item->getMaterial());
      _player_invincible_effect->remove(player_item->getMaterial());
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

      _player_invincible_effect->add(player_item->getMaterial());
   }
   else if (skull_type == Constants::SkullInvisible)
   {
      _invisible_player_effect->addPlayer(player_item);
   }
   else
   {
      _player_infected_effect->add(player_item->getMaterial());
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::createMapItem(MapItem *item)
{
   if (!_playfield)
      return;

   if (!_map_items.contains(item))
   {
      Mesh *mesh= nullptr;

      switch (item->getType())
      {
         case MapItem::Stone:
         {
            mesh= createBlock(_playfield, _stones, item->getX(), item->getY(), 0.8f);
            _stone_list.push_back(item);
            break;
         }

         case MapItem::Block:
         {
            mesh= createBlock(_playfield, _blocks, item->getX(), item->getY(), 0.9f);
            break;
         }

         case MapItem::Bomb:
         {
            mesh = createBomb(item);
            break;
         }

         case MapItem::Extra:
         {
            ExtraMapItem *extra= dynamic_cast<ExtraMapItem*>(item);

            if (extra->getExtraType() == Constants::ExtraSkull)
            {
               mesh = createSkull(item);
               mesh = nullptr; // skulls are tracked in _skull_map, not _meshes
            }
            else
               mesh = createExtra(extra);

            break;
         }

         default:
            item= nullptr;
            break;
      }

      if (item)
      {
         _map_items.insert(item);

         if (mesh)
         {
            _meshes[item] = mesh;

            if (
                  item->getType() == MapItem::Stone
               || item->getType() == MapItem::Block
            )
            {
               addBlock(item);
            }
         }
      }
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::removeMapItem(MapItem *item)
{
   std::unordered_set<MapItem*>::iterator it= _map_items.find(item);

   if (it != _map_items.end())
   {
      std::erase(_stone_list, item);

      _shaking_boxes.erase(item);

      // get associated mesh
      auto m= _meshes.find(item);

      if (m != _meshes.end())
      {
         if (
               item->getType() == MapItem::Stone
            || item->getType() == MapItem::Block
         )
         {
            removeBlock(item);
         }

         Mesh *mesh= m->second;

         _stones->removeMesh(mesh);
         _shadow_blocks->removeMesh(mesh);
         _blocks->removeMesh(mesh);
         _extra_bomb->removeMesh(mesh);
         _extra_flame->removeMesh(mesh);
         _extra_speedup->removeMesh(mesh);
         _extra_kick->removeMesh(mesh);
         _bombs->removeMesh(mesh);
         _shadow_billboards->removeMesh(mesh);
         _fuse_particle_system->removeEmitter(item);
         deleteMesh(mesh);
         _meshes.erase(m);
      }
      else
      {
         auto si= _skull_map.find(item);

         if (si != _skull_map.end())
         {
            Mesh *skull_mesh= si->second;

            _skulls->removeMesh(skull_mesh);
            deleteMesh(skull_mesh);
            _skull_map.erase(si);
         }
      }

      _map_items.erase(it);
   }
}


//-----------------------------------------------------------------------------
/*!
*/
Node* GameDrawable::createDestruction(SceneGraph *scene, float x, float y, Constants::Direction direction, float flame_count)
{
   Dummy *dummy= nullptr;

   // create destruction animation
   Node *root= nullptr;
   if (flame_count < 3)
      root= _destruct_anim[0];
   else if (flame_count < 5)
      root= _destruct_anim[1];
   else if (flame_count < 8)
      root= _destruct_anim[2];
   else
      root= _destruct_anim[3];

   if (root)
   {
      int rot= 0;
      switch (direction)
      {
         case Constants::DirectionUp:    rot= 1; break;
         case Constants::DirectionDown:  rot= 3; break;
         case Constants::DirectionLeft:  rot= 2; break;
         case Constants::DirectionRight: rot= 0; break;
         default: break;
      }
      Node* destruct= root->getChild(rot);

      dummy= new Dummy(scene);
      dummy->setUserTransformable(true);
      Matrix pos;
      pos= Matrix::rotateZ( rot * std::numbers::pi_v<float> * 0.5f );
      pos.translate( Vector(x+0.5f, -y-0.5f) );
      dummy->setUserTransformable(true);
      Matrix scale= Matrix::scale(0.8f, 0.8f, 0.8f);
      dummy->setTransform(scale * pos);

      for (int i=0; i<destruct->getChildCount(); i++)
      {
         Node* child= destruct->getChild(i);
         if (child->id() == Node::idMesh)
         {
            Mesh *ref= dynamic_cast<Mesh*>(child);
            Mesh *mesh= new Mesh(*ref, dummy);
            mesh->setUserTransformable(false);
            _destruction->addMesh(mesh);
            mesh->setFrame(0.0f);
         }
      }
   }

   return dummy;
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::destroyMapItem(MapItem *item, float flame_count)
{
   std::unordered_set<MapItem*>::iterator it= _map_items.find(item);
   if (it!=_map_items.end())
   {
      if (item->getType() == MapItem::Stone)
      {
         Node* dummy=
            createDestruction(
                _playfield,
                item->getX(),
                item->getY(),
                item->getDestroyDirection(),
                flame_count
            );

         if (dummy)
            _destructions.push_back(dummy);
      }
   }

   removeMapItem(item);
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::addDetonation(int x, int y, int up, int down, int left, int right, float intense)
{
   if (_detonations)
   {
      intense= std::sqrt(intense)*0.15f;
      if (intense>0.5f)
         intense= 0.5f;
      if (intense > _bounce)
         _bounce= intense;
      _detonations->addDetonation(x,y,up,down,left,right);
   }
}


//-----------------------------------------------------------------------------
/*!
   \param item item to update
   \param x x position
   \param y y position
   \param z z position
*/
void GameDrawable::setMapItemPosition(
   MapItem * item,
   float x,
   float y,
   float z
)
{
   auto iter = _meshes.find(item);

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

      Mesh* mesh = iter->second;

      Matrix pos;
      pos.identity();

      pos.translate(
         Vector(
            x + 0.5f,
           -y - 0.5f,
            z
         )
      );

      mesh->setTransform(pos);
   }
}


//-----------------------------------------------------------------------------
/*!
   \param x x position where extra has been removed
   \param y y position where extra has been removed
   \param destroyed \c true if extra was destroyed
   \param player_id if of player who picked the extra up
*/
void GameDrawable::extraRemoved(
   int x,
   int y,
   bool destroyed,
   Constants::ExtraType extra,
   int player_id
)
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
      PlayerItem* player = getPlayer(player_id);

      if (player)
         player->setFlash(1.0f);
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

   PlayerItem* player= getPlayer(id);

   if (player)
   {
      player->setRotation(angle + std::numbers::pi_v<float> * 0.5f);
      player->setPosition(x, y);
   }
}


//-----------------------------------------------------------------------------
/*!
   set zoom factor
   \param zoom zoom factor
*/
void GameDrawable::setCameraZoom(float zoom)
{
   _camera_zoom= zoom;
}


//-----------------------------------------------------------------------------
/*!
   \param width dimension width
   \param height dimension height
   \return dimensions enum
*/
Constants::Dimension GameDrawable::getDimensions(
   float& width,
   float& height
) const
{
   GameInformation* info =
      BombermanClient::getInstance()->getCurrentGameInformation();

   if (!info)
   {
      width= 0.0f;
      height= 0.0f;
      return Constants::DimensionInvalid;
   }

   Constants::Dimension dimensions = info->getMapDimensions();

   switch (dimensions)
   {
      case Constants::Dimension13x11:
         width = 13.0f;
         height = 11.0f;
         break;

      case Constants::Dimension19x17:
         width = 19.0f;
         height = 17.0f;
         break;

      case Constants::Dimension25x21:
         width = 25.0f;
         height = 21.0f;
         break;

      default:
         break;
   }

   return dimensions;
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::setPlayerSpeed(int id, float dx, float dy, float /*da*/)
{
   PlayerItem *player= getPlayer(id);
   if (player)
   {
      player->setSpeed( std::sqrt(dx*dx+dy*dy) );
   }
}


//-----------------------------------------------------------------------------
/*!
*/
PlayerItem* GameDrawable::getPlayer(int id) const
{
   auto it= _player_list.find(id);
   if (it != _player_list.end())
      return it->second;
   else
      return nullptr;
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::addPlayer(int id, const std::string& nick, Constants::Color color)
{
   PlayerItem *player= getPlayer(id);
   if (player)
   {
      qWarning("GameDrawable::addPlayer: player already exists");
      player->setKilled(false);
      return;
   }

   player= new PlayerItem(id, nick, color);
   _player_list[id] = player;

   Mesh *mesh= MotionMixer::getMesh("bomberman");
   if (!mesh)
   {
      qWarning("GameDrawable::addPlayer: mesh not found");
   }

   Mesh *p= new Mesh(_players);
   p->copy(*mesh);
   MotionMixer *mixer= new MotionMixer();
   p->setMotionMixer(mixer);
   p->setVisible(true);

   player->setMesh(p);
   Material* player_material= _players->getMaterial(static_cast<int32_t>(color-1));
   player_material->addMesh(p);
   player->setMaterial( player_material );

   _shadow_billboards->addMesh(p);
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::removePlayer(int id)
{
   PlayerItem *player= getPlayer(id);

   if (player)
   {
      player->kill();
      _player_infected_effect->remove(player->getMaterial());
      _invisible_player_effect->removePlayer(player);
   }

   if (id == _player_id && _mushroom_animation->isActive())
   {
      _mushroom_animation->abort();
   }

   // check for survivors
   if (_player_list.size() > 1)
   {
      int alive= 0;
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
   switch (GameStateMachine::getInstance()->getState())
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

   float delta= time - _time;
   _time= time;

   if (_player_death_effect)
      _player_death_effect->animate(delta);

   _player_infected_effect->animate(delta);

   _fuse_particle_system->animate(delta);
   _player_invincible_effect->animate(delta);
   _star_talers_factory->update(delta * 0.05f);

   _camera_anim+=delta*60.0f;

   if (_bounce > delta*0.01f)
      _bounce-=delta*0.01f;
   else
      _bounce= 0.0f;

   // rotational rotation is rotating:
   for (const auto& [item, item_mesh] : _meshes)
   {
      switch (item->getType())
      {
         case MapItem::Extra:
         {
            Extra *extra= dynamic_cast<Extra*>(item_mesh);
            extra->animate(time);
         }
         break;

         case MapItem::Bomb:
         {
            Mesh *mesh= item_mesh;
            float t= time * 0.1f + mesh->getAnimationFrame();

            Vector pos= mesh->getTransform().translation();

            _fuse_particle_system->setEmitterPosition(item, pos + FuseParticleSystem::getBombOffset());

            float sx= 1.0f + std::sin(t)*0.2f;
            float sy= 1.0f - std::sin(t)*0.3f;

            Matrix mat= Matrix::scale(sx,sx,sy);
            mat.translate(pos);
            mesh->setTransform(mat);
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
   for (std::vector<Node*>::iterator it= _destructions.begin(); it!=_destructions.end(); )
   {
      Node* destr= *it;
      bool remove= false;
      for (int i=0; i<destr->getChildCount(); i++)
      {
         Mesh *mesh= dynamic_cast<Mesh*>(destr->getChild(i));
         float frame= mesh->getFrame() + delta * 30.0f;
         if (frame > 4000)
            remove= true;
         mesh->setFrame(frame);
      }

      if (remove)
      {
         it= _destructions.erase(it);
         for (int i=0; i<destr->getChildCount(); i++)
         {
            Mesh *mesh= dynamic_cast<Mesh*>(destr->getChild(i));
            _destruction->removeMesh(mesh);
            deleteMesh(mesh);
         }
         delete destr;
      }
      else
         it++;
   }

   if (_level)
      _level->animate(delta);

   animateSkulls(time);
}


//-----------------------------------------------------------------------------
/*!
*/
void GameDrawable::shakeBoxes(float delta)
{
   std::unordered_map<MapItem*,float>::iterator it;
   for (it= _shaking_boxes.begin(); it!=_shaking_boxes.end();)
   {
      float time= it->second;
      time-=delta;
      if (time<0.0f) time= 0.0f;
      float intense= time*time;
      MapItem* item= it->first;
      Mesh* mesh= getMesh(item);
      if (mesh)
      {
         const Matrix& cur= mesh->getTransform();

         Matrix scale;
         float x= 0.8f + std::sin((1.0f-time) * 14.0f) * 0.1f * intense;
         float y= 0.8f - std::sin((1.0f-time) * 12.0f) * 0.1f * intense;
         float z= 0.6f + std::cos((1.0f-time) * 16.0f) * 0.2f * intense + 0.2f *(1.0f - intense);

         scale= Matrix::scale(x, y, z);
         scale.translate( cur.translation() );
         mesh->setTransform(scale);
      }

      if (!mesh || time <= 0.0f)
         it= _shaking_boxes.erase(it);
      else
      {
         it->second= time;
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
   float time= GlobalTime::Instance()->getTime();

   _invisible_player_effect->update(time);

   float dt = _time - _time_prev;

   shakeBoxes(dt*0.015f);

   float width, height;
   Constants::Dimension dimensions;
   dimensions = getDimensions(width, height);

   float bounce_x, bounce_y;

   bounce_x= _camera_shake_intensity * 0.5f * std::sin(time * 71.0f) * _bounce;
   bounce_y= _camera_shake_intensity * 0.5f * std::sin(time * 113.0f) * _bounce;

   if (dimensions == Constants::Dimension19x17)
   {
       bounce_x *= 0.66f;
       bounce_y *= 0.66f;
   }

   Matrix view;

   if (_level)
   {
      _level->startPositionUpdate(width, height, dt);

      if (_camera_follows_player)
      {
         // real HeadlessIntegration (bot/multi-instance camera) branch deferred - always follows
         // this client's own player, matching the single-player-on-this-pc path.
         PlayerInfo* player= BombermanClient::getInstance()->getCurrentPlayerInfo();
         if (player)
            _level->addPlayerPosition( player );

         // if no players have been added to the camera interpolation; then add all players
         if (_level->isPlayerMapEmpty())
         {
            const auto* players =
               BombermanClient::getInstance()->getPlayerInfoMap();

            for (const auto& [player_id, p] : *players)
               _level->addPlayerPosition( p );
         }
      }

      _level->endPlayerPositionUpdate();

      view= _level->getCameraMatrix(_camera_anim, _camera_zoom);
   }

   Matrix shake= Matrix::position(bounce_x, bounce_y, 0.0f) * view;

   // clears the frame, space draws its starfield and earth here
   if (_level)
   {
      _level->drawBackground();
   }

   // render scene
   if (_level_scene_graph)
   {
      _level_scene_graph->render(_camera_anim, shake);
   }

   if (_playfield)
   {
      _playfield->render(0.0, shake);
   }

   if (_players)
   {
      _invisible_player_effect->captureBackground();
      _players->render(0.0, shake);
   }

   _detonations->render();
   _fuse_particle_system->render();
   _player_invincible_effect->render();
   _star_talers_factory->render();

   // start flow fields when a player got killed (and the kill anim is over) - matches the
   // original's own "player_mesh->getFrame() > 10000.0f" convention: PlayerItem::animate() only
   // wraps the frame counter back to 0 while alive, so it climbs unbounded once mKilled is set,
   // naturally crossing 10000 once the one-shot death animation has long finished playing.
   for (const auto& [player_id, player] : _player_list)
   {
      if (player->isKilled())
      {
         Mesh* player_mesh = player->getMesh();

         if (player_mesh->getFrame() > 10000.0f)
         {
            Geometry* player_geometry = player_mesh->getPart(0);

            if (player_geometry->isVisible())
            {
               Material* material = _players->getMaterial(static_cast<int32_t>(player->getColor()) - 1);
               _player_death_effect->add(material);
               player_geometry->setVisible(false);
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
      _shroom_filter->setTime(GlobalTime::Instance()->getTime());
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

   auto it= _player_list.begin();
   while (it != _player_list.end())
   {
      PlayerItem* player= it->second;
      it= _player_list.erase(it);

      int color= static_cast<int32_t>(player->getColor());
      Mesh *mesh= player->getMesh();
      Material* player_material= _players->getMaterial(static_cast<int32_t>(color-1));
      player_material->removeMesh(mesh);
      _shadow_billboards->removeMesh(mesh);

      delete player;
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
