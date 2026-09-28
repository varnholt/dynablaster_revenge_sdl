// header
#include "levelcastle.h"
#include "snowanimation.h"

#include <format>

// engine/nodes
#include "nodes/scenegraph.h"
#include "nodes/camera.h"
#include "nodes/mesh.h"
#include "nodes/dummy.h"

// engine/materials
#include "materials/texturematerial.h"
#include "materials/displacementmaterial.h"
#include "materials/environmentmaterial.h"
#include "materials/environmenttexturematerial.h"
#include "materials/environmentambientdiffusematerial.h"

#include "materials/blockmaterial.h"
#include "materials/extramapping.h"
#include "materials/invisibilitymaterial.h"
#include "materials/shadowbillboard.h"
#include "materials/destructionmaterial.h"
#include "materials/playermaterial.h"
#include "materials/skullmaterial.h"

// engine/render
#include "render/texturepool.h"

// framework
#include "gldevice.h"

// cmath
#include <math.h>

LevelCastle::LevelCastle()
 : Level(Level::LevelCastle),
   _snow_animation(std::make_unique<SnowAnimation>())
{
}


//-----------------------------------------------------------------------------
/*!
*/
LevelCastle::~LevelCastle() = default;


//-----------------------------------------------------------------------------
/*!
*/
void LevelCastle::initialize()
{
}


//-----------------------------------------------------------------------------
/*!
*/
void LevelCastle::draw()
{
   // the snow's unit square spans the level, -8 compensates the wind drift
   Matrix transform = Matrix::scale(15.0f, 15.0f, 15.0f);
   transform.translate(Vector(-8.0f, -17.0f, 0.0f));
   _snow_animation->draw(transform);
}


//-----------------------------------------------------------------------------
/*!
*/
void LevelCastle::animate(float dt)
{
   _snow_animation->animate(dt);
}


//-----------------------------------------------------------------------------
/*!
*/
void LevelCastle::reset()
{
   _snow_animation->reset();
}


Material* LevelCastle::createMaterial(SceneGraph* scene, int id) const
{
   Material* mat= 0;
   switch(id)
   {
      case (MAP_DIFFUSE):
         mat= new TextureMaterial(scene);
         break;
      case (MAP_REFLECT):
         mat= new EnvironmentMaterial(scene);
         break;
      case (MAP_DIFFUSE | MAP_REFLECT):
         mat= new EnvironmentTextureMaterial(scene);
         break;
      case (MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT):
         mat= new EnvironmentAmbientDiffuseMaterial(scene);
         break;
      case (MAP_DIFFUSE | MAP_DISPLACE):
         mat= new DisplacementMaterial(scene);
         break;
      default:
         mat= 0;
         break;
   }
   return mat;
}

void LevelCastle::loadData()
{
   Camera *shadowCam = 0;

   if (!isAborted())
   {
      mLevel = new SceneGraph();
      mScene = new SceneGraph();
      mPlayers = new SceneGraph();

      mLevel->load("level.hjb", this);

      Camera* center= (Camera*)mLevel->getNode("Camera001");
      Camera* upper= (Camera*)mLevel->getNode("Upper");
      Camera* lower= (Camera*)mLevel->getNode("Lower");
      mCamInterp= new CameraInterpolation(center, upper, lower);
   }

   if (!isAborted())
   {
      shadowCam= (Camera*)mLevel->getNode("Shadow Cam");
      shadowCam->setPerspectiveMode(false);
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
      loadDestructions(shadowCam);
   }

   if (!isAborted())
   {
      mShadowBillboards= new ShadowBillboard(mScene, "shadowdrop-3x3");
      mShadowBlocks= new ShadowBillboard(mScene, "shadow_block");

      mBombs= new EnvironmentTextureMaterial(
         mScene,
         "bomb",
         "diffuse_level",
         "diffuse_bomb"
      );

      mStones= new BlockMaterial(
         mScene,
         "stone-unwrap",
         "diffuse_level",
         "specular_level",
         "shadow-cookie",
         shadowCam
      );

      mBlocks= new BlockMaterial(
         mScene,
         "block-unwrap",
         "diffuse_level",
         "specular_level",
         "shadow-cookie",
         shadowCam
      );

      mSkulls= new SkullMaterial(
         mScene,
         "skull",
         "diffuse_level",
         "specular_level",
         "shadow-cookie",
         shadowCam
      );

      Material* material = 0;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
         std::string filename = std::format("player_{}", i + 1);
         material= new PlayerMaterial(
               mPlayers,
               filename.c_str(),
               "diffuse_level",
               "specular_level2",
               "player-ao"
         );
         (void)material;
      }

      material= new InvisibilityMaterial( mPlayers );

      mExtraFlame   = new ExtraMapping(mScene, "extra_flame");
      mExtraBomb    = new ExtraMapping(mScene, "extra_bomb");
      mExtraSpeedup = new ExtraMapping(mScene, "extra_speedup");
      mExtraKick    = new ExtraMapping(mScene, "extra_kick");
      mExtraSkull   = new ExtraMapping(mScene, "extra_skull");
   }

   TexturePool::Instance()->update();

   if (!isAborted())
   {
      mLevel->setCamera(0);
      mScene->setCamera(0);
      mPlayers->setCamera(0);
   }
}


//-----------------------------------------------------------------------------
/*!
*/
std::string LevelCastle::getLensFlareKey() const
{
   return "castle";
}
