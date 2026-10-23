#include "levelpirate.h"

#include "nodes/camera.h"
#include "nodes/scenegraph.h"

#include "materials/blockmaterial.h"
#include "materials/environmenttexturematerial.h"
#include "materials/extramapping.h"
#include "materials/invisibilitymaterial.h"
#include "materials/playermaterial.h"
#include "materials/shadowbillboard.h"
#include "materials/skullmaterial.h"

#include "engine/gltf/gltfmodel.h"
#include "engine/render/glowsprites.h"
#include "engine/render/watersurface.h"
#include "framework/gldevice.h"
#include "framework/globaltime.h"
#include "render/texturepool.h"

#include <format>

LevelPirate::LevelPirate() : Level(Level::LevelPirate)
{
}

LevelPirate::~LevelPirate() = default;

void LevelPirate::loadData()
{
   std::optional<std::reference_wrapper<Camera>> shadow_camera;

   if (!isAborted())
   {
      _level = std::make_unique<SceneGraph>();
      _scene = std::make_unique<SceneGraph>();
      _players = std::make_unique<SceneGraph>();

      // cameras only, the ship itself is drawn by drawLevel()
      _level->load("level.hjb");
      _ship = std::make_unique<GltfModel>("ship.glb");

      const auto center = _level->findNode<Camera>("Camera001");
      const auto upper = _level->findNode<Camera>("Upper");
      const auto lower = _level->findNode<Camera>("Lower");
      _camera_interpolation = std::make_unique<CameraInterpolation>(center, upper, lower);

      // the fly-in shows the whole ship for a while, the game's cameras take over by the end of the countdown
      _camera_interpolation->setIntroTimes(15000.0f, 22500.0f);

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

void LevelPirate::drawLevel(float /*frame*/, const Matrix& view)
{
   // the camera is all in the view matrix, the level's scene graph has none
   _level->setupCamera(view);

   if (!_ship->isValid())
   {
      return;
   }

   const GltfModel::Look look;
   const float time = GlobalTime::Instance().getTime();

   // the ship and the sky in the sea
   if (_ship->hasWater())
   {
      if (!_water)
      {
         _water = std::make_unique<WaterSurface>();
      }

      _water->renderReflection(
         _ship->getWaterLevel(),
         [&](const Matrix& mirror)
         {
            GltfModel::begin();
            _ship->draw(mirror, -1, time, look);
            GltfModel::end();
         }
      );
   }

   GltfModel::begin();
   _ship->draw(Matrix(), -1, time, look);
   GltfModel::end();

   if (_water)
   {
      // the camera sits where the inverted view projection puts the clip space's z axis
      const Matrix inverse = static_cast<GLDevice&>(activeDevice()).getProjectionMatrix().invert4x4();
      const Vector eye(inverse.xz / inverse.wz, inverse.yz / inverse.wz, inverse.zz / inverse.wz);

      _water->captureRefraction();
      _water->render(*_ship, eye);
   }

   // the lanterns and the moon
   if (!_ship->getGlows().empty())
   {
      if (!_glows)
      {
         _glows = std::make_unique<GlowSprites>();
         _glows->set(_ship->getGlows());
      }

      _glows->render(Matrix());
   }
}
