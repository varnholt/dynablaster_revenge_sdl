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

Material* LevelCastle::createMaterial(SceneGraph* scene, int id) const
{
   switch (id)
   {
      case MAP_DIFFUSE:
      {
         return new TextureMaterial(scene);
      }
      case MAP_REFLECT:
      {
         return new EnvironmentMaterial(scene);
      }
      case MAP_DIFFUSE | MAP_REFLECT:
      {
         return new EnvironmentTextureMaterial(scene);
      }
      case MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT:
      {
         return new EnvironmentAmbientDiffuseMaterial(scene);
      }
      case MAP_DIFFUSE | MAP_DISPLACE:
      {
         return new DisplacementMaterial(scene);
      }
      default:
      {
         return nullptr;
      }
   }
}

void LevelCastle::loadData()
{
   Camera* shadow_camera = nullptr;

   if (!isAborted())
   {
      _level = std::make_unique<SceneGraph>();
      _scene = std::make_unique<SceneGraph>();
      _players = std::make_unique<SceneGraph>();

      _level->load("level.hjb", this);

      auto* center = static_cast<Camera*>(_level->getNode("Camera001"));
      auto* upper = static_cast<Camera*>(_level->getNode("Upper"));
      auto* lower = static_cast<Camera*>(_level->getNode("Lower"));
      _camera_interpolation = std::make_unique<CameraInterpolation>(center, upper, lower);
   }

   if (!isAborted())
   {
      shadow_camera = static_cast<Camera*>(_level->getNode("Shadow Cam"));
      shadow_camera->setPerspectiveMode(false);
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
      loadDestructions(shadow_camera);
   }

   if (!isAborted())
   {
      _shadow_billboards = new ShadowBillboard(_scene.get(), "shadowdrop-3x3");
      _shadow_blocks = new ShadowBillboard(_scene.get(), "shadow_block");

      _bombs = new EnvironmentTextureMaterial(_scene.get(), "bomb", "diffuse_level", "diffuse_bomb");
      _stones = new BlockMaterial(_scene.get(), "stone-unwrap", "diffuse_level", "specular_level", "shadow-cookie", shadow_camera);
      _blocks = new BlockMaterial(_scene.get(), "block-unwrap", "diffuse_level", "specular_level", "shadow-cookie", shadow_camera);
      _skulls = new SkullMaterial(_scene.get(), "skull", "diffuse_level", "specular_level", "shadow-cookie", shadow_camera);

      // the scene graph takes ownership of every material created for it
      for (int32_t i = 0; i < MAX_PLAYERS; i++)
      {
         const std::string filename = std::format("player_{}", i + 1);
         new PlayerMaterial(_players.get(), filename.c_str(), "diffuse_level", "specular_level2", "player-ao");
      }

      new InvisibilityMaterial(_players.get());

      _extra_flame = new ExtraMapping(_scene.get(), "extra_flame");
      _extra_bomb = new ExtraMapping(_scene.get(), "extra_bomb");
      _extra_speedup = new ExtraMapping(_scene.get(), "extra_speedup");
      _extra_kick = new ExtraMapping(_scene.get(), "extra_kick");
      _extra_skull = new ExtraMapping(_scene.get(), "extra_skull");
   }

   TexturePool::Instance()->update();

   if (!isAborted())
   {
      _level->setCamera(nullptr);
      _scene->setCamera(nullptr);
      _players->setCamera(nullptr);
   }
}

std::string LevelCastle::getLensFlareKey() const
{
   return "castle";
}
