#include "gamehelpdrawable.h"

#include "framework/gldevice.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "menus/bitmapfont.h"
#include "menus/defaultshader.h"
#include "menus/fontpool.h"
#include "menus/psdlayer.h"

#include "helpmanager.h"
#include "soundmanager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include <ranges>
#include <string_view>

namespace
{
constexpr float SCREEN_WIDTH = 1920.0f;
constexpr float SCREEN_HEIGHT = 1080.0f;
constexpr float SCREEN_OFFSET_TOP = 100.0f;
constexpr float TEXT_HEADLINE_SCALE = 0.15f;
constexpr float TEXT_FONT_SCALE = 0.10f;
constexpr float HALF_PI = std::numbers::pi_v<float> * 0.5f;

bool equalsIgnoreCase(std::string_view a, std::string_view b)
{
   return std::ranges::equal(
      a, b, [](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); }
   );
}

// up to three lines, separated by ';'
std::vector<std::string> splitLines(std::string_view message)
{
   std::vector<std::string> lines;
   for (const auto line : std::views::split(message, ';'))
   {
      lines.emplace_back(line.begin(), line.end());
   }
   lines.resize(3);
   return lines;
}
}  // namespace

GameHelpDrawable::GameHelpDrawable(RenderDevice& dev) : Drawable(dev)
{
   HelpManager::getInstance().messageAddedSignal.connect(
      [this](const std::string& page, const std::string& message, Constants::HelpSeverity severity, Constants::HelpLocation location, int delay)
      { showHelp(page, message, severity, location, delay); }
   );
}

GameHelpDrawable::~GameHelpDrawable()
{
}

void GameHelpDrawable::initializeGL()
{
   initializeLayers();
   _font = FontPool::Instance().get("default").value();
}

void GameHelpDrawable::initializeLayers()
{
   _psd.load("data/errorhandler/errorhandler.psd");

   for (auto& psd_layer : _psd.getLayers())
   {
      auto layer = std::make_unique<PSDLayer>(psd_layer);
      const std::string& name = psd_layer.getName();

      if (name == "icon_info")
      {
         _layer_icon_info = *layer;
      }
      else if (name == "sprite_info")
      {
         _layer_sprite_info = *layer;
      }
      else if (name == "icon_error")
      {
         _layer_icon_error = *layer;
      }
      else if (name == "sprite_error")
      {
         _layer_sprite_error = *layer;
      }
      else if (name == "error_bubble")
      {
         _layer_bubble = *layer;
      }
      else if (name == "text_offset_title")
      {
         _layer_line_title = *layer;
      }
      else if (name == "text_offset_2")
      {
         _layer_line_2 = *layer;
      }
      else if (name == "text_offset_3")
      {
         _layer_line_3 = *layer;
      }

      _psd_layers.push_back(std::move(layer));
   }
}

void GameHelpDrawable::showHelp(
   const std::string& page,
   const std::string& message,
   Constants::HelpSeverity severity,
   Constants::HelpLocation location,
   int delay
)
{
   // errors and undelayed messages jump the queue
   if (severity == Constants::HelpSeverityError || delay == 0)
   {
      _help_items.emplace_front(page, message, severity, location, delay);
   }
   else
   {
      _help_items.emplace_back(page, message, severity, location, delay);
   }
}

void GameHelpDrawable::pageChanged(const std::string& page)
{
   _page = page;
}

void GameHelpDrawable::updateHelpItems()
{
   while (!_help_items.empty())
   {
      HelpElement& help = _help_items.front();

      if (help.isStarted())
      {
         if (help.isFinished())
         {
            _help_items.pop_front();
            continue;
         }
         return;
      }

      // a message for another page is obsolete
      if (!help.getPage().empty() && !equalsIgnoreCase(help.getPage(), _page))
      {
         _help_items.pop_front();
         continue;
      }

      if (help.isDelayElapsed())
      {
         help.start();
         playSound(help);
      }
      else if (!help.isDelayStarted())
      {
         help.startDelay();
      }

      return;
   }
}

void GameHelpDrawable::drawQuad(PSDLayer& layer, float left, float top, float alpha, float scale_x, float scale_y)
{
   // grows the quad by scale_x/scale_y on every side
   const float width = static_cast<float>(layer.getWidth());
   const float height = static_cast<float>(layer.getHeight());

   Matrix world = Matrix::scale((width + 2.0f * scale_x) / width, (height + 2.0f * scale_y) / height, 1.0f);
   world.translate(Vector(left - scale_x, top - scale_y, 0.0f));

   layer.render(world, alpha);
}

void GameHelpDrawable::paintGL()
{
   updateHelpItems();

   if (_help_items.empty() || !_help_items.front().isStarted())
   {
      return;
   }

   const HelpElement& help = _help_items.front();
   const bool notification = (help.getSeverity() == Constants::HelpSeverityNotification);

   PSDLayer& sprite = notification ? *_layer_sprite_info : *_layer_sprite_error;
   PSDLayer& icon = notification ? *_layer_icon_info : *_layer_icon_error;
   PSDLayer& bubble = *_layer_bubble;

   const float elapsed = help.getElapsed();
   const float fade_in = std::min(1.0f, elapsed / HelpElement::fade_in_duration);
   const float fade_out =
      std::min(1.0f, (elapsed - HelpElement::idle_duration - HelpElement::fade_in_duration) / HelpElement::fade_out_duration);
   const bool fading_in = elapsed <= HelpElement::fade_in_duration;
   const bool fading_out = elapsed > HelpElement::fade_in_duration + HelpElement::idle_duration;

   float fade = 1.0f;
   if (fading_in)
   {
      fade = std::sin(fade_in * HALF_PI);
   }
   else if (fading_out)
   {
      fade = std::cos(fade_out * HALF_PI);
   }

   const bool align_right =
      (help.getLocation() == Constants::HelpLocationButtomRight || help.getLocation() == Constants::HelpLocationTopRight);
   const bool align_top = (help.getLocation() == Constants::HelpLocationTopLeft || help.getLocation() == Constants::HelpLocationTopRight);

   // where the psd's origin lands on screen
   const float offset_x = align_right ? SCREEN_WIDTH - _psd.getWidth() : 0.0f;
   const float offset_y = align_top ? SCREEN_OFFSET_TOP : SCREEN_HEIGHT - SCREEN_OFFSET_TOP;

   initGlParameters();

   // the sprite slides in from the screen edge
   const float sprite_x = align_right ? SCREEN_WIDTH - sprite.getWidth() * fade : 0.0f;
   drawQuad(sprite, sprite_x, offset_y, 1.0f);

   // the bubble pops up once the sprite is in
   if (elapsed > HelpElement::fade_in_duration)
   {
      const float t = std::min(1.0f, (elapsed - HelpElement::fade_in_duration) / HelpElement::popup_duration);
      const float amplitude = 1.0f - t;
      const float wobble = (1.0f + std::sin(50.0f * t) * 0.75f) * 0.5f;
      const float scale = (wobble * amplitude + (t - 1.0f) * 0.3f) * 0.3f;

      const float alpha = 1.0f - fade_out;
      const float text_alpha = fading_out ? std::cos(fade_out * HALF_PI) : 1.0f;

      drawQuad(
         bubble, offset_x + bubble.getLeft(), offset_y + bubble.getTop(), alpha, scale * bubble.getWidth(), scale * bubble.getHeight()
      );
      drawQuad(icon, offset_x + icon.getLeft(), offset_y + icon.getTop(), alpha, scale * icon.getWidth(), scale * icon.getHeight());

      BitmapFont& font = *_font;
      const PSDLayer& line_title = *_layer_line_title;
      const PSDLayer& line_2 = *_layer_line_2;
      const PSDLayer& line_3 = *_layer_line_3;

      if (notification)
      {
         font.setColor(0.055f, 0.365f, 0.514f, text_alpha);
      }
      else
      {
         font.setColor(0.369f, 0.0f, 0.0f, text_alpha);
      }

      font.buildVertices(
         TEXT_HEADLINE_SCALE, notification ? "Information" : "Error", offset_x + line_title.getLeft(), offset_y + line_title.getTop()
      );
      font.draw();

      // like the original, each line sits on the next line's offset (text is drawn above its y)
      const std::vector<std::string> lines = splitLines(help.getMessage());
      const float line_x = offset_x + line_2.getLeft();
      const float line_2_y = offset_y + line_2.getTop();
      const float line_3_y = offset_y + line_3.getTop();

      font.setColor(0.2f, 0.2f, 0.2f, text_alpha);
      font.buildVertices(TEXT_FONT_SCALE, lines[0], line_x, line_2_y);
      font.draw();
      font.buildVertices(TEXT_FONT_SCALE, lines[1], line_x, line_3_y);
      font.draw();
      font.buildVertices(TEXT_FONT_SCALE, lines[2], line_x, line_3_y + (line_3_y - line_2_y));
      font.draw();

      font.setColor(1.0f, 1.0f, 1.0f, 1.0f);
   }

   cleanupGlParameters();
}

void GameHelpDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice().setShader(getDefaultMenuShader());
}

void GameHelpDrawable::cleanupGlParameters()
{
   activeDevice().setShader(0);

   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void GameHelpDrawable::playSound(const HelpElement& element)
{
   if (element.getSeverity() == Constants::HelpSeverityError)
   {
      SoundManager::getInstance().playSoundError();
   }
   else
   {
      SoundManager::getInstance().playSoundInfo();
   }
}
