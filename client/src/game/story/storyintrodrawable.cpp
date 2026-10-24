#include "storyintrodrawable.h"

#include "engine/gltf/gltfmodel.h"
#include "engine/render/glowsprites.h"
#include "engine/render/nightsky.h"
#include "framework/gldevice.h"
#include "log.h"
#include "tools/datapaths.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>
#include <sstream>

namespace
{
constexpr const char* TIMELINE = "story/intro/intro.txt";
constexpr const char* MODELS = "story/intro/";
constexpr const char* SKY = "story/intro/sky";
constexpr float FADE_IN = 1.5f;
constexpr float FADE_OUT = 1.2f;
constexpr float Z_NEAR = 0.3f;
constexpr float Z_FAR = 400.0f;

// moonlight on the scene
const Vector NIGHT(0.72f, 0.78f, 0.98f);

Matrix readMatrix(std::istream& in)
{
   std::array<float, 16> m{};

   for (float& value : m)
   {
      in >> value;
   }

   // column by column, i.e. in the order of the members
   return Matrix(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
}
}  // namespace

StoryIntroDrawable::StoryIntroDrawable(RenderDevice& device) : Drawable(device)
{
}

StoryIntroDrawable::~StoryIntroDrawable() = default;

void StoryIntroDrawable::initializeGL()
{
}

bool StoryIntroDrawable::load()
{
   if (_loaded || _load_failed)
   {
      return _loaded;
   }

   auto file = DataPaths::open(TIMELINE);

   if (!file.is_open())
   {
      Log::warning(std::format("StoryIntroDrawable: {} not found", TIMELINE));
      _load_failed = true;
      return false;
   }

   // whitespace separated:
   //   fps <fps> frames <n>
   //   actor <name> <model> <from> <until> <action count> <matrix count>
   //      action <start> <name> <loop|hold>  (action count times)
   //      <16 floats, column by column>       (matrix count times)
   //   camera <n>
   //      <horizontal fov in degrees> <16 floats>  (n times)
   std::stringstream in;
   in << file.rdbuf();

   std::string word;
   int32_t frames = 0;

   while (in >> word)
   {
      if (word == "fps")
      {
         in >> _fps;
      }
      else if (word == "frames")
      {
         in >> frames;
      }
      else if (word == "actor")
      {
         Actor& actor = _actors.emplace_back();
         std::string model;
         size_t action_count = 0;
         size_t matrix_count = 0;
         in >> actor._name >> model >> actor._from >> actor._until >> action_count >> matrix_count;
         actor._model = std::make_unique<GltfModel>(MODELS + model);

         for (size_t i = 0; i < action_count; i++)
         {
            Action& action = actor._actions.emplace_back();
            std::string mode;
            in >> word >> action._start >> action._name >> mode;
            action._loop = mode == "loop";
            action._animation = actor._model->findAnimation(action._name);
            action._duration = actor._model->getAnimationDuration(action._animation);

            if (action._animation < 0)
            {
               Log::warning(std::format("StoryIntroDrawable: {} has no animation {}", model, action._name));
            }
         }

         for (size_t i = 0; i < matrix_count; i++)
         {
            actor._matrices.push_back(readMatrix(in));
         }
      }
      else if (word == "camera")
      {
         size_t count = 0;
         in >> count;

         for (size_t i = 0; i < count; i++)
         {
            float fov = 0.0f;
            in >> fov;
            _fov.push_back(fov);
            _camera.push_back(readMatrix(in));
         }
      }
   }

   if (in.bad() || _camera.empty() || _actors.empty())
   {
      Log::warning(std::format("StoryIntroDrawable: unable to read {}", TIMELINE));
      _actors.clear();
      _load_failed = true;
      return false;
   }

   _duration = static_cast<float>(frames) / _fps;

   // the lights of the lab and the castle's windows, all of them standing still
   std::vector<GltfModel::Glow> glows;
   for (const Actor& actor : _actors)
   {
      if (actor._matrices.size() == 1)
      {
         for (GltfModel::Glow glow : actor._model->getGlows())
         {
            glow._position = actor._matrices.front() * glow._position;
            glows.push_back(glow);
         }
      }
   }

   if (!glows.empty())
   {
      _glows = std::make_unique<GlowSprites>();
      _glows->set(glows);
   }

   _sky = std::make_unique<NightSky>(SKY);

   _loaded = true;
   return true;
}

void StoryIntroDrawable::play(std::function<void()> finished)
{
   _finished = std::move(finished);
   setVisible(true);

   if (!load())
   {
      finish();
      return;
   }

   _time = 0.0f;
   _last_time = 0.0f;
}

void StoryIntroDrawable::skip()
{
   if (isPlaying())
   {
      finish();
   }
}

bool StoryIntroDrawable::isPlaying() const
{
   return _time >= 0.0f;
}

void StoryIntroDrawable::finish()
{
   _time = -1.0f;

   if (_finished)
   {
      // the callback may play it again
      auto finished = std::move(_finished);
      _finished = nullptr;
      finished();
   }
}

void StoryIntroDrawable::animate(float global_time)
{
   // global time comes in 16ms steps
   const float dt = _last_time > 0.0f ? (global_time - _last_time) * 0.016f : 0.0f;
   _last_time = global_time;

   if (!isPlaying())
   {
      return;
   }

   _time += dt;

   if (_time >= _duration)
   {
      finish();
   }
}

Matrix StoryIntroDrawable::sample(const std::vector<Matrix>& matrices, float time) const
{
   if (matrices.size() == 1)
   {
      return matrices.front();
   }

   const float frame = std::clamp(time * _fps, 0.0f, static_cast<float>(matrices.size() - 1));
   const auto index = static_cast<size_t>(frame);
   const size_t next = std::min(index + 1, matrices.size() - 1);

   return Matrix::blend(matrices[index], matrices[next], frame - static_cast<float>(index));
}

float StoryIntroDrawable::getFade() const
{
   if (!isPlaying())
   {
      return 0.0f;
   }

   return std::clamp(std::min(_time / FADE_IN, (_duration - _time) / FADE_OUT), 0.0f, 1.0f);
}

void StoryIntroDrawable::paintGL()
{
   GLDevice& device = static_cast<GLDevice&>(activeDevice());
   const float fade = getFade();

   device.clear(0.0f, 0.0f, 0.0f, 1.0f);

   if (!isPlaying() || fade <= 0.0f)
   {
      return;
   }

   // the camera looks down its -z like the scene graph's, its fov is horizontal over a 16:9 frame
   const Matrix view = sample(_camera, _time).invert();
   const auto frame = std::min(static_cast<size_t>(_time * _fps), _fov.size() - 1);
   const float half_fov = _fov[frame] * 0.5f * std::numbers::pi_v<float> / 180.0f;
   device.setCamera(view, std::tan(half_fov) * 9.0f / 16.0f, Z_NEAR, Z_FAR);

   _sky->setBrightness(fade);
   _sky->render();

   GltfModel::Look look;
   look._tint = NIGHT * fade;

   GltfModel::setDaylight(1.0f);
   GltfModel::begin();

   for (const Actor& actor : _actors)
   {
      if (_time < actor._from || _time >= actor._until)
      {
         continue;
      }

      // the last action started so far
      int32_t animation = -1;
      float time = 0.0f;

      for (const Action& action : actor._actions)
      {
         if (_time >= action._start && action._animation >= 0)
         {
            animation = action._animation;
            time = _time - action._start;

            // draw() wraps the time, those that hold stop just short of their end
            if (!action._loop)
            {
               time = std::min(time, std::max(action._duration - 0.001f, 0.0f));
            }
         }
      }

      actor._model->draw(sample(actor._matrices, _time), animation, time, look);
   }

   GltfModel::end();

   if (_glows)
   {
      _glows->setBrightness(fade);
      _glows->render(Matrix());
   }
}
