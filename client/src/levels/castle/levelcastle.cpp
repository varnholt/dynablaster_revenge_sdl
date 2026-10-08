#include "levelcastle.h"
#include "snowanimation.h"

// engine/nodes
#include "nodes/camera.h"
#include "nodes/dummy.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"

// engine/materials
#include "materials/blockmaterial.h"
#include "materials/destructionmaterial.h"
#include "materials/displacementmaterial.h"
#include "materials/environmentambientdiffusematerial.h"
#include "materials/environmentmaterial.h"
#include "materials/environmenttexturematerial.h"
#include "materials/extramapping.h"
#include "materials/invisibilitymaterial.h"
#include "materials/playermaterial.h"
#include "materials/shadowbillboard.h"
#include "materials/skullmaterial.h"
#include "materials/texturematerial.h"

// engine/render
#include "render/texturepool.h"

// framework
#include "gldevice.h"

#include <format>
#include <utility>

#include "constants.h"

LevelCastle::LevelCastle() : Level(Level::LevelCastle), _snow_animation(std::make_unique<SnowAnimation>())
{
}

LevelCastle::~LevelCastle() = default;

void LevelCastle::initialize()
{
}

void LevelCastle::draw()
{
   // the snow's unit square spans the level, -8 compensates the wind drift
   Matrix transform = Matrix::scale(15.0f, 15.0f, 15.0f);
   transform.translate(Vector(-8.0f, -17.0f, 0.0f));
   _snow_animation->draw(transform);
}

void LevelCastle::animate(float dt)
{
   _snow_animation->animate(dt);
}

void LevelCastle::reset()
{
   _snow_animation->reset();
}

std::unique_ptr<Material> LevelCastle::createMaterial(int32_t id) const
{
   switch (id)
   {
      case MAP_DIFFUSE:
      {
         return std::make_unique<TextureMaterial>();
      }
      case MAP_REFLECT:
      {
         return std::make_unique<EnvironmentMaterial>();
      }
      case MAP_DIFFUSE | MAP_REFLECT:
      {
         return std::make_unique<EnvironmentTextureMaterial>();
      }
      case MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT:
      {
         return std::make_unique<EnvironmentAmbientDiffuseMaterial>();
      }
      case MAP_DIFFUSE | MAP_DISPLACE:
      {
         return std::make_unique<DisplacementMaterial>();
      }
      default:
      {
         return nullptr;
      }
   }
}

void LevelCastle::loadData()
{
   std::optional<std::reference_wrapper<Camera>> shadow_camera;

   if (!isAborted())
   {
      _level = std::make_unique<SceneGraph>();
      _scene = std::make_unique<SceneGraph>();
      _players = std::make_unique<SceneGraph>();

      _level->load("level.hjb", *this);

      const auto center = _level->findNode<Camera>("Camera001");
      const auto upper = _level->findNode<Camera>("Upper");
      const auto lower = _level->findNode<Camera>("Lower");
      _camera_interpolation = std::make_unique<CameraInterpolation>(center, upper, lower);
   }

   if (!isAborted())
   {
      shadow_camera = _level->findNode<Camera>("Shadow Cam");
      shadow_camera->get().setPerspectiveMode(false);
   }

   if (!isAborted())
   {
      _scene->load("block.hjb");
      _scene->load("bomb.hjb");
      _scene->load("extra.hjb");
      _scene->load("skull.hjb");
   }

   if (!isAborted())
   {
      loadDestructions(*shadow_camera);
   }

   if (!isAborted())
   {
      _shadow_billboards = _scene->addMaterial(std::make_unique<ShadowBillboard>("shadowdrop-3x3"));
      _shadow_blocks = _scene->addMaterial(std::make_unique<ShadowBillboard>("shadow_block"));

      _bombs = _scene->addMaterial(std::make_unique<EnvironmentTextureMaterial>("bomb", "diffuse_level", "diffuse_bomb"));
      _stones = _scene->addMaterial(
         std::make_unique<BlockMaterial>("stone-unwrap", "diffuse_level", "specular_level", "shadow-cookie", *shadow_camera)
      );
      _blocks = _scene->addMaterial(
         std::make_unique<BlockMaterial>("block-unwrap", "diffuse_level", "specular_level", "shadow-cookie", *shadow_camera)
      );
      _skulls =
         _scene->addMaterial(std::make_unique<SkullMaterial>("skull", "diffuse_level", "specular_level", "shadow-cookie", *shadow_camera));

      // the scene graph takes ownership of every material created for it
      for (int32_t i = 0; i < MAX_PLAYERS; i++)
      {
         const std::string filename = std::format("player_{}", i + 1);
         _players->addMaterial(std::make_unique<PlayerMaterial>(filename, "diffuse_level", "specular_level2", "player-ao"));
      }

      _players->addMaterial(std::make_unique<InvisibilityMaterial>());

      _extra_flame = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_flame"));
      _extra_bomb = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_bomb"));
      _extra_speedup = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_speedup"));
      _extra_kick = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_kick"));
      _extra_skull = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_skull"));

      // story mode
      const std::pair<int32_t, const char*> story_extras[] = {
         {Constants::ExtraWallPass, "extra_wallpass"},
         {Constants::ExtraBombPass, "extra_bombpass"},
         {Constants::ExtraRemote, "extra_remote"},
         {Constants::ExtraVest, "extra_vest"},
         {Constants::ExtraOneUp, "extra_oneup"},
         {Constants::ExtraExit, "extra_exit"},
      };
      for (const auto& [type, texture] : story_extras)
      {
         _story_extras[type] = _scene->addMaterial(std::make_unique<ExtraMapping>(texture));
      }
   }

   TexturePool::Instance().update();

   if (!isAborted())
   {
      _level->clearCamera();
      _scene->clearCamera();
      _players->clearCamera();
   }
}

std::string LevelCastle::getLensFlareKey() const
{
   return "castle";
}
