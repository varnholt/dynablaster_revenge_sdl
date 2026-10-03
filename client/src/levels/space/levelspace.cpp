#include "levelspace.h"
#include "spacebackground.h"

#include "nodes/camera.h"
#include "nodes/scenegraph.h"

#include "materials/blockmaterial.h"
#include "materials/environmentmaterial.h"
#include "materials/environmenttexturematerial.h"
#include "materials/extramapping.h"
#include "materials/invisibilitymaterial.h"
#include "materials/playermaterial.h"
#include "materials/shadowbillboard.h"
#include "materials/skullmaterial.h"
#include "materials/texturematerial.h"

#include "render/texturepool.h"

#include <format>

LevelSpace::LevelSpace() : Level(Level::LevelSpace)
{
}

LevelSpace::~LevelSpace() = default;

void LevelSpace::drawBackground()
{
   _background->draw();
}

void LevelSpace::animate(float dt)
{
   _background->animate(dt);
}

std::string LevelSpace::getLensFlareKey() const
{
   return "space";
}

std::unique_ptr<Material> LevelSpace::createMaterial(int32_t id) const
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
      default:
      {
         return nullptr;
      }
   }
}

void LevelSpace::loadData()
{
   std::optional<std::reference_wrapper<Camera>> shadow_camera;

   if (!isAborted())
   {
      _background = std::make_unique<SpaceBackground>();
   }

   if (!isAborted())
   {
      _level = std::make_unique<SceneGraph>();
      _scene = std::make_unique<SceneGraph>();
      _players = std::make_unique<SceneGraph>();

      _level->load("level.hjb", *this);
   }

   if (!isAborted())
   {
      shadow_camera = _level->findNode<Camera>("Shadow Cam");
      shadow_camera->get().setPerspectiveMode(false);

      const auto center = _level->findNode<Camera>("Camera001");
      const auto upper = _level->findNode<Camera>("Upper");
      const auto lower = _level->findNode<Camera>("Lower");
      _camera_interpolation = std::make_unique<CameraInterpolation>(center, upper, lower);
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
      _skulls = _scene->addMaterial(std::make_unique<SkullMaterial>("skull", "diffuse_level", "specular_level", "", *shadow_camera));

      // the scene graph takes ownership of every material created for it
      for (int32_t i = 0; i < MAX_PLAYERS; i++)
      {
         const std::string filename = std::format("player_{}", i + 1);
         _players->addMaterial(std::make_unique<PlayerMaterial>(filename, "diffuse_level", "specular_level", "player-ao"));
      }

      _players->addMaterial(std::make_unique<InvisibilityMaterial>());

      _extra_flame = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_flame"));
      _extra_bomb = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_bomb"));
      _extra_speedup = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_speedup"));
      _extra_kick = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_kick"));
      _extra_skull = _scene->addMaterial(std::make_unique<ExtraMapping>("extra_skull"));
   }

   TexturePool::Instance().update();

   if (!isAborted())
   {
      _level->clearCamera();
      _scene->clearCamera();
      _players->clearCamera();
   }
}
