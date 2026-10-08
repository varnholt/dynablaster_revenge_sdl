#include "storyhuddrawable.h"

#include "framework/gldevice.h"
#include "math/matrix.h"
#include "menus/bitmapfont.h"
#include "menus/defaultshader.h"
#include "menus/fontpool.h"

// shared
#include "storystage.h"
#include "storystatepacket.h"

#include <algorithm>
#include <array>
#include <format>

namespace
{
constexpr float SCREEN_WIDTH = 1920.0f;
constexpr float SCREEN_HEIGHT = 1080.0f;
constexpr float BANNER_TIME = 3.0f;
constexpr float FADE_TIME = 0.4f;

constexpr std::array<std::string_view, 8> WORLD_NAMES = {
   "The Wall",
   "Rocky Mountains",
   "River",
   "Forest",
   "Lava Cave",
   "Inside of the Castle I",
   "Inside of the Castle II",
   "Inside of the Castle III"
};
}  // namespace

StoryHudDrawable::StoryHudDrawable(RenderDevice& device) : Drawable(device)
{
}

void StoryHudDrawable::initializeGL()
{
   _font = &FontPool::Instance().get("outlined").value().get();
}

void StoryHudDrawable::setState(int32_t stage, int32_t lives, int32_t score, int32_t state, int32_t enemies_left)
{
   const auto& stages = getStoryStages();
   const std::string name = stage >= 0 && stage < static_cast<int32_t>(stages.size()) ? std::string(stages[static_cast<size_t>(stage)]._name) : "";
   const bool changed = state != _state || stage != _stage;

   _stage = stage;
   _lives = lives;
   _score = score;
   _enemies_left = enemies_left;
   _state = state;

   if (!changed)
   {
      return;
   }

   switch (state)
   {
      case StoryStatePacket::StateStageIntro:
         showBanner(std::format("STAGE {}", name), std::string(WORLD_NAMES[static_cast<size_t>(std::clamp(stage / 8, 0, 7))]));
         break;
      case StoryStatePacket::StateStageCleared:
         showBanner("STAGE CLEAR", std::format("SCORE {}", score));
         break;
      case StoryStatePacket::StateGameOver:
         showBanner("GAME OVER", std::format("PASSWORD {}", stages[static_cast<size_t>(stage)]._password));
         break;
      case StoryStatePacket::StateCompleted:
         showBanner("CONGRATULATIONS", std::format("FINAL SCORE {}", score));
         break;
      default:
         break;
   }
}

void StoryHudDrawable::showBanner(const std::string& title, const std::string& subtitle)
{
   _banner_title = title;
   _banner_subtitle = subtitle;
   _banner_time = 0.0f;
}

void StoryHudDrawable::animate(float global_time)
{
   // global time comes in 16ms steps
   const float dt = _last_time > 0.0f ? (global_time - _last_time) * 0.016f : 0.0f;
   _last_time = global_time;

   if (_banner_time >= 0.0f)
   {
      _banner_time += dt;

      if (_banner_time > BANNER_TIME)
      {
         _banner_time = -1.0f;
      }
   }
}

void StoryHudDrawable::paintGL()
{
   if (!_font || _state < 0)
   {
      return;
   }

   static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(Matrix::ortho(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f));

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   activeDevice().setShader(getDefaultMenuShader());

   const auto& stages = getStoryStages();
   const std::string_view name = stages[static_cast<size_t>(std::clamp(_stage, 0, static_cast<int32_t>(stages.size()) - 1))]._name;

   BitmapFont& font = *_font;
   font.setColor(1.0f, 1.0f, 1.0f, 1.0f);
   font.buildVertices(0.16f, std::format("STAGE {}", name), 40.0f, 40.0f);
   font.draw();
   font.buildVertices(0.16f, std::format("LIVES {}", _lives), 40.0f, 95.0f);
   font.draw();
   font.buildVertices(0.16f, std::format("SCORE {}", _score), 40.0f, 150.0f);
   font.draw();
   font.setColor(1.0f, 0.75f, 0.3f, 1.0f);
   font.buildVertices(0.12f, std::format("ENEMIES {}", _enemies_left), 40.0f, 205.0f);
   font.draw();

   if (_banner_time >= 0.0f)
   {
      const float fade_in = std::min(_banner_time / FADE_TIME, 1.0f);
      const float fade_out = std::clamp((BANNER_TIME - _banner_time) / FADE_TIME, 0.0f, 1.0f);
      const float alpha = std::min(fade_in, fade_out);

      font.setColor(1.0f, 1.0f, 1.0f, alpha);
      font.buildVertices(0.45f, _banner_title, 0.0f, SCREEN_HEIGHT * 0.42f, SCREEN_WIDTH);
      font.draw();
      font.setColor(1.0f, 0.85f, 0.4f, alpha);
      font.buildVertices(0.2f, _banner_subtitle, 0.0f, SCREEN_HEIGHT * 0.56f, SCREEN_WIDTH);
      font.draw();
   }

   font.setColor(1.0f, 1.0f, 1.0f, 1.0f);

   activeDevice().setShader(0);
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}
