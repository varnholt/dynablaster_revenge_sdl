#include "levelmansion.h"

#include "nodes/camera.h"
#include "nodes/scenegraph.h"

#include "materials/blockmaterial.h"
#include "materials/environmentambientdiffusematerial.h"
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

LevelMansion::LevelMansion() : Level(Level::LevelMansion)
{
}

std::string LevelMansion::getLensFlareKey() const
{
   return "mansion";
}

Material* LevelMansion::createMaterial(SceneGraph* scene, int id) const
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
      default:
      {
         return nullptr;
      }
   }
}

void LevelMansion::loadData()
{
   Camera* shadow_camera = nullptr;

   if (!isAborted())
   {
      mLevel = new SceneGraph();
      mScene = new SceneGraph();
      mPlayers = new SceneGraph();

      mLevel->load("level.hjb", this);
   }

   if (!isAborted())
   {
      shadow_camera = static_cast<Camera*>(mLevel->getNode("Shadow Cam"));
      shadow_camera->setPerspectiveMode(false);

      auto* camera = static_cast<Camera*>(mLevel->getCamera());
      camera->setNear(5.0f);
      camera->setFar(200.0f);

      auto* center = static_cast<Camera*>(mLevel->getNode("Camera001"));
      auto* upper = static_cast<Camera*>(mLevel->getNode("Upper"));
      auto* lower = static_cast<Camera*>(mLevel->getNode("Lower"));
      mCamInterp = new CameraInterpolation(center, upper, lower);
   }

   if (!isAborted())
   {
      mScene->load("block.hjb");
      mScene->load("bomb.hjb");
      mScene->load("extra.hjb");
      mScene->load("skull.hjb");
   }

   if (!isAborted())
   {
      loadDestructions(shadow_camera);
   }

   if (!isAborted())
   {
      auto* shadow_billboards = new ShadowBillboard(mScene, "shadowdrop-3x3");
      shadow_billboards->setOffset(0.0f, 0.3f);
      mShadowBillboards = shadow_billboards;
      mShadowBlocks = new ShadowBillboard(mScene, "shadow_block");

      mBombs = new EnvironmentTextureMaterial(mScene, "bomb", "diffuse_level", "diffuse_bomb");
      mStones = new BlockMaterial(mScene, "stone-unwrap", "diffuse_level", "specular_level2", "shadow-cookie", shadow_camera);
      mBlocks = new BlockMaterial(mScene, "block-unwrap", "diffuse_level", "specular_level2", "shadow-cookie", shadow_camera);
      mSkulls = new SkullMaterial(mScene, "skull", "diffuse_level", "specular_level", "shadow-cookie", shadow_camera);

      // the scene graph takes ownership of every material created for it
      for (int32_t i = 0; i < MAX_PLAYERS; i++)
      {
         const std::string filename = std::format("player_{}", i + 1);
         new PlayerMaterial(mPlayers, filename.c_str(), "diffuse_level", "specular_level", "player-ao");
      }

      new InvisibilityMaterial(mPlayers);

      mExtraFlame = new ExtraMapping(mScene, "extra_flame");
      mExtraBomb = new ExtraMapping(mScene, "extra_bomb");
      mExtraSpeedup = new ExtraMapping(mScene, "extra_speedup");
      mExtraKick = new ExtraMapping(mScene, "extra_kick");
      mExtraSkull = new ExtraMapping(mScene, "extra_skull");
   }

   TexturePool::Instance()->update();

   if (!isAborted())
   {
      mLevel->setCamera(nullptr);
      mScene->setCamera(nullptr);
      mPlayers->setCamera(nullptr);
   }
}
