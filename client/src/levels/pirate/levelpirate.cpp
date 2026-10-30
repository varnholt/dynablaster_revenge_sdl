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
#include "engine/render/seaspray.h"
#include "engine/render/watersurface.h"
#include "framework/gldevice.h"
#include "framework/globaltime.h"
#include "render/texturepool.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>
#include <string_view>

namespace
{
// the light probes from day to night (assets/level-pirate/build_all.py's NIGHT_STEPS)
constexpr int32_t NIGHT_STEPS = 4;

// the ship's side at the waterline, either side of her centre line through the field's middle (build_pirate.py's
// YC and the sea's foam along the hull), and the stretch of her waist and stern the waves break against
constexpr float CENTRE_Y = -5.5f;
constexpr float HULL = 9.6f;
constexpr float SPLASH_X0 = -8.0f;
constexpr float SPLASH_X1 = 22.0f;

// the wind over the deck as she sails towards -x, it carries the spray aft
const Vector WIND(2.5f, 0.4f, 0.0f);

// the sea's swell round the ship: the transform that moves what floats on it, and the roll alone round the swell's
// pivot (on the moved surface) with its inverse, for the mirror image in the moved surface
struct Swell
{
   Matrix _transform;
   Matrix _roll;
   Matrix _roll_inverse;
   float _heave = 0.0f;
};

// it heaves and rolls about her keel (along x), pitching a little along her: three slow beats that never quite
// repeat, round the field's middle at the waterline so the field's surroundings move least
Swell swellAt(float time, float level)
{
   constexpr float tau = 2.0f * std::numbers::pi_v<float>;
   constexpr float degree = std::numbers::pi_v<float> / 180.0f;
   const float roll = 0.6f * degree * (0.8f * std::sin(time * tau / 7.3f) + 0.2f * std::sin(time * tau / 3.1f + 1.3f));
   const float pitch = 0.25f * degree * std::sin(time * tau / 9.7f + 0.7f);
   const float heave = 0.14f * std::sin(time * tau / 6.1f + 2.1f);

   const Vector pivot(6.5f, CENTRE_Y, level + heave);
   Swell swell;
   swell._heave = heave;
   swell._roll = Matrix::position(-pivot) * Matrix::rotateX(roll) * Matrix::rotateY(pitch) * Matrix::position(pivot);
   swell._roll_inverse = Matrix::position(-pivot) * Matrix::rotateY(-pitch) * Matrix::rotateX(-roll) * Matrix::position(pivot);
   swell._transform = Matrix::position(0.0f, 0.0f, heave) * swell._roll;
   return swell;
}
}  // namespace

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

void LevelPirate::setDayTime(float phase)
{
   // (phase, daylight): morning, noon, afternoon, sunset, night, dawn
   constexpr std::array<std::array<float, 2>, 7> day{{
      {0.0f, 0.8f},
      {0.25f, 1.0f},
      {0.5f, 0.9f},
      {0.62f, 0.45f},
      {0.72f, 0.0f},
      {0.94f, 0.0f},
      {1.0f, 0.35f},
   }};

   phase = std::clamp(phase, 0.0f, 1.0f);
   for (size_t i = 0; i + 1 < day.size(); i++)
   {
      if (phase <= day[i + 1][0])
      {
         const float t = (phase - day[i][0]) / (day[i + 1][0] - day[i][0]);
         const float smooth = t * t * (3.0f - 2.0f * t);
         _day = day[i][1] + (day[i + 1][1] - day[i][1]) * smooth;
         break;
      }
   }

   // the warm light while the sun is low
   _dusk = std::max(0.0f, 1.0f - std::fabs(_day - 0.45f) / 0.35f) * (phase > 0.5f || phase < 0.1f ? 1.0f : 0.0f);

   // the blocks and the players lit by the evening's light probes, a step at a time as it gets dark
   const int32_t step = static_cast<int32_t>(std::lround((1.0f - _day) * static_cast<float>(NIGHT_STEPS)));
   if (step != _probe_step)
   {
      _probe_step = step;
      const auto probe = [this, step](std::string_view name)
      { return step > 0 ? std::format("data/{}/{}_{}", path(), name, step) : std::format("data/{}/{}", path(), name); };
      setLightProbes(probe("diffuse_level"), probe("specular_level"), probe("specular_level"), std::format("data/{}/diffuse_bomb", path()));
   }
}

void LevelPirate::splash(float time, float level)
{
   if (!_spray)
   {
      _spray = std::make_unique<SeaSpray>();
   }

   const float dt = _last_time < 0.0f ? 0.0f : std::clamp(time - _last_time, 0.0f, 0.1f);
   _last_time = time;

   std::uniform_real_distribution<float> unit(0.0f, 1.0f);
   if (time >= _next_splash)
   {
      // not on the first frame: the first wave comes after a while
      if (_next_splash >= 0.0f)
      {
         // mostly against her far side, whose spray the game's camera sees rising past the rail, now and then
         // against the near one for the fly-in
         const float side = unit(_random) < 0.75f ? 1.0f : -1.0f;
         const Vector at(SPLASH_X0 + (SPLASH_X1 - SPLASH_X0) * unit(_random), CENTRE_Y + side * HULL, level);
         _spray->burst(at, Vector(0.0f, side, 0.0f), 0.7f + 0.6f * unit(_random), WIND);
         _splashes.push_back({time, at});
      }

      _next_splash = time + 8.0f + 7.0f * unit(_random);
   }

   _spray->animate(dt, level);

   // the broken wave's foam on the water for a few seconds, a few rings along the hull
   std::erase_if(_splashes, [time](const Splash& splash) { return time - splash._time > 3.0f; });
   if (_water)
   {
      std::vector<WaterSurface::Wader> foam;
      for (const Splash& splash : _splashes)
      {
         for (const float along : {-0.7f, 0.0f, 0.7f})
         {
            foam.push_back({Vector2(splash._at.x + along, splash._at.y), Vector2(0.0f, 0.0f)});
         }
      }
      _water->setWaders(std::move(foam));
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

   GltfModel::Look look;
   look._tint = Vector(1.0f, 1.0f - 0.12f * _dusk, 1.0f - 0.3f * _dusk);
   const float time = GlobalTime::Instance().getTime();

   // the gulls circling overhead, the ship's one animation
   const int32_t gulls = _ship->findAnimation("gulls");

   // the sea moves round the ship, which keeps still with the field on its deck
   const float level = _ship->getWaterLevel();
   const Swell swell = swellAt(time, level);
   GltfModel::setSwell(swell._transform);

   // the ship's light by day and by night blended, other glTF models are always drawn by day
   GltfModel::setDaylight(_day);

   // the ship's sun, and by day the light the sea throws up her hull
   GltfModel::Atmosphere air = _ship->getAtmosphere();
   air._caustics.x *= _day;
   GltfModel::setAtmosphere(air);

   // the ship and the sky in the sea, mirrored in its moved surface
   if (_ship->hasWater())
   {
      if (!_water)
      {
         _water = std::make_unique<WaterSurface>();
      }

      _water->renderReflection(
         level + swell._heave,
         [&](const Matrix& mirror)
         {
            GltfModel::begin();
            _ship->draw(swell._roll_inverse * mirror * swell._roll, gulls, time, look);
            GltfModel::end();
         }
      );
   }

   GltfModel::begin();
   _ship->draw(Matrix(), gulls, time, look);
   GltfModel::end();

   splash(time, level);

   if (_water)
   {
      // the camera sits where the inverted view projection puts the clip space's z axis
      const Matrix inverse = static_cast<GLDevice&>(activeDevice()).getProjectionMatrix().invert4x4();
      const Vector eye(inverse.xz / inverse.wz, inverse.yz / inverse.wz, inverse.zz / inverse.wz);

      // the moon's path on the sea at night, fainter than the sun's
      GltfModel::Atmosphere sea = air;
      sea._sun = air._sun * (0.3f + 0.7f * _day);
      GltfModel::setAtmosphere(sea);

      _water->captureRefraction();
      _water->render(*_ship, eye, swell._transform);
   }

   // the spray as white as the light lets it be
   const Vector night(0.2f, 0.25f, 0.35f);
   _spray->render(swell._transform, night + (Vector(0.95f, 0.97f, 1.0f) - night) * _day);

   GltfModel::setDaylight(1.0f);
   GltfModel::setSwell(Matrix());

   // the blocks drawn after take nothing of the sea's light
   GltfModel::setAtmosphere({});

   // the lanterns, and the moon in the sky
   if (!_ship->getGlows().empty())
   {
      if (!_glows)
      {
         std::vector<GltfModel::Glow> lanterns;
         std::vector<GltfModel::Glow> sky;
         for (const GltfModel::Glow& glow : _ship->getGlows())
         {
            (glow._position.length() > 100.0f ? sky : lanterns).push_back(glow);
         }

         _glows = std::make_unique<GlowSprites>();
         _glows->set(lanterns);
         _sky_glows = std::make_unique<GlowSprites>();
         _sky_glows->set(sky);
      }

      // the lanterns are lit towards the evening
      const float brightness = std::clamp(1.0f - _day * 1.25f, 0.0f, 1.0f);
      _glows->setBrightness(brightness);
      _glows->render(Matrix());
      _sky_glows->setBrightness(brightness);
      _sky_glows->render(swell._transform);
   }
}
