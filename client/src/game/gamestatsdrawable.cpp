#include "gamestatsdrawable.h"

#include "framework/gldevice.h"
#include "framework/globaltime.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "menus/bitmapfont.h"
#include "menus/defaultshader.h"
#include "menus/fontpool.h"
#include "menus/psdlayer.h"

#include "bombermanclient.h"
#include "gamestatemachine.h"
#include "playerinfo.h"
#include "soundmanager.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string_view>

namespace
{
constexpr std::string_view LAYER_PLAYER_ICON = "player_icon_";
constexpr std::string_view LAYER_GAME_TIME_PANEL_ORB = "time-panel-orb";
constexpr std::string_view LAYER_GAME_TIME_ORB = "time-orb";
constexpr std::string_view LAYER_GAME_TIME = "time";
constexpr std::string_view LAYER_MUTE_MUSIC = "music-mute-icon";
constexpr std::string_view LAYER_MUTE_SFX = "sfx-mute-icon";
constexpr std::string_view LAYER_SKULL_BLUE = "blue-skull";
constexpr std::string_view LAYER_SKULL_RED = "red-skull";
constexpr std::string_view LAYER_SKULL_PURPLE = "purple-skull";
constexpr std::string_view LAYER_SKULL_GREEN = "green-skull";
constexpr std::string_view LAYER_SKULL_GOLD = "gold-skull";
constexpr std::string_view LAYER_SKULL_ALL = "all-skull";
constexpr std::string_view LAYER_SKULL_PROGRESS_START = "progressbar-start";
constexpr std::string_view LAYER_SKULL_PROGRESS_MIDDLE = "progressbar-middle";

// the player icons line up right to left
constexpr float PLAYER_ICON_OFFSET = 10.0f;
constexpr float OFFSET_Y_PLAYER_NICK = 15.0f;
constexpr float OFFSET_Y_PLAYER_WINS = 10.0f;
constexpr float OFFSET_X_PLAYER_KILLS = 40.0f;
constexpr float OFFSET_Y_PLAYER_KILLS = 35.0f;
constexpr float OFFSET_X_PLAYER_DEATHS = -40.0f;
constexpr float OFFSET_Y_PLAYER_DEATHS = 35.0f;
constexpr float FONT_SCALE_NICK = 0.085f;
constexpr float FONT_SCALE_WINS = 0.15f;
constexpr float FONT_SCALE_STATS = 0.085f;
constexpr float FONT_SCALE_TIME = 0.30f;
}  // namespace

GameStatsDrawable::GameStatsDrawable(RenderDevice& dev) : Drawable(dev)
{
   _filename = "data/game/gameui.psd";
}

GameStatsDrawable::~GameStatsDrawable()
{
}

void GameStatsDrawable::initializeGL()
{
   GameStateMachine::getInstance().stateChangedSignal.connect([this]() { resetGrayscales(); });

   _time_font = FontPool::Instance().get("time").value();
   _outline_font = FontPool::Instance().get("outlined").value();

   initializeLayers();
   _orb_palette.load("data/game/game_orb");

   _grayscale_shader = activeDevice().loadShader("data/shaders/grayscale-vert.glsl", "data/shaders/grayscale-frag.glsl");
   _param_grayscale = activeDevice().getParameterIndex("intensity");

   _progress_shader = activeDevice().loadShader("data/shaders/progressbar-vert.glsl", "data/shaders/progressbar-frag.glsl");
   _param_progress = activeDevice().getParameterIndex("progress");

   _color_shader = activeDevice().loadShader("data/shaders/texcolor-vert.glsl", "data/shaders/texcolor-frag.glsl");
   _param_color = activeDevice().getParameterIndex("color");
}

void GameStatsDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   for (auto& psd_layer : _psd.getLayers())
   {
      auto layer = std::make_unique<PSDLayer>(psd_layer);
      const std::string_view name = psd_layer.getName();

      if (name.starts_with(LAYER_PLAYER_ICON))
      {
         const int number = std::stoi(std::string(name.substr(LAYER_PLAYER_ICON.size()))) - 1;
         if (number >= 0 && number < static_cast<int>(_layers_player.size()))
         {
            _layers_player[number] = *layer;
         }
      }
      else if (name == LAYER_GAME_TIME)
      {
         _game_time_offset_x = static_cast<float>(psd_layer.getLeft());
         _game_time_offset_y = static_cast<float>(psd_layer.getTop() + psd_layer.getHeight());
         _game_time_height = static_cast<float>(psd_layer.getHeight());
      }
      else if (name == LAYER_GAME_TIME_ORB)
      {
         _layer_orb = *layer;
      }
      else if (name == LAYER_GAME_TIME_PANEL_ORB)
      {
         _layer_panel_orb = *layer;
      }
      else if (name == LAYER_MUTE_MUSIC)
      {
         _layer_mute_music = *layer;
      }
      else if (name == LAYER_MUTE_SFX)
      {
         _layer_mute_sfx = *layer;
      }
      else if (name == LAYER_SKULL_RED)
      {
         _layers_skulls[Constants::SkullAutofire] = *layer;
      }
      else if (name == LAYER_SKULL_BLUE)
      {
         _layers_skulls[Constants::SkullMinimumBomb] = *layer;
      }
      else if (name == LAYER_SKULL_PURPLE)
      {
         _layers_skulls[Constants::SkullKeyboardInvert] = *layer;
      }
      else if (name == LAYER_SKULL_ALL)
      {
         _layers_skulls[Constants::SkullMushroom] = *layer;
      }
      else if (name == LAYER_SKULL_GREEN)
      {
         _layers_skulls[Constants::SkullInvisible] = *layer;
      }
      else if (name == LAYER_SKULL_GOLD)
      {
         _layers_skulls[Constants::SkullInvincible] = *layer;
      }
      else if (name == LAYER_SKULL_PROGRESS_START)
      {
         _layer_skull_progress_start = *layer;
      }
      else if (name == LAYER_SKULL_PROGRESS_MIDDLE)
      {
         _layer_skull_progress_middle = *layer;
      }

      _psd_layers.push_back(std::move(layer));
   }
}

void GameStatsDrawable::paintGL()
{
   initGlParameters();

   updateMuteIcons();
   drawOverlay();
   drawPlayerIcons();
   drawPlayerInfo();
   drawGameTime();
   drawSkullProgress();
   drawOrb();
   drawSkull();

   cleanupGlParameters();
}

void GameStatsDrawable::animate(float global_time)
{
   if (std::fabs(_last_global_time - global_time) > 0.0001f)
   {
      if (_last_global_time != 0.0f)
      {
         const float dt = global_time - _last_global_time;
         animateDeathGrayScales(dt);
         animateSkulls(dt);
      }

      _last_global_time = global_time;
   }
}

void GameStatsDrawable::animateDeathGrayScales(float dt)
{
   for (const PlayerInfo& info : BombermanClient::getInstance().getPlayerInfoList())
   {
      if (info.isKilled())
      {
         float& grayscale = _grayscales[info.getColor()];
         grayscale = std::min(1.0f, grayscale + 0.01f * dt);
      }
   }
}

void GameStatsDrawable::animateSkulls(float dt)
{
   if (_skull_aborted)
   {
      _skull_alpha -= dt * 0.01f;
   }
}

void GameStatsDrawable::drawOverlay()
{
   for (const auto& layer : _psd_layers)
   {
      if (layer->getLayer().isVisible())
      {
         layer->render();
      }
   }
}

void GameStatsDrawable::drawPlayerIcons()
{
   float x = 0.0f;

   for (const PlayerInfo& info : BombermanClient::getInstance().getPlayerInfoList())
   {
      const int color = static_cast<int>(info.getColor()) - 1;
      if (color < 0 || color >= static_cast<int>(_layers_player.size()) || !_layers_player[color])
      {
         continue;
      }

      if (info.isKilled())
      {
         activeDevice().setShader(_grayscale_shader);
         activeDevice().setParameter(_param_grayscale, _grayscales[info.getColor()]);
      }

      PSDLayer& layer = *_layers_player[color];
      layer.render(x);

      if (info.isKilled())
      {
         activeDevice().setShader(getDefaultMenuShader());
      }

      x -= layer.getWidth() + PLAYER_ICON_OFFSET;
   }
}

void GameStatsDrawable::drawPlayerInfo()
{
   if (!_layers_player[0])
   {
      return;
   }

   // all player icon layers share one spot, the icons are moved left from there
   const PSDLayer& layer = *_layers_player[0];
   BitmapFont& font = *_outline_font;

   float x = 0.0f;

   for (const PlayerInfo& info : BombermanClient::getInstance().getPlayerInfoList())
   {
      // dead players' stats fade to grey
      float grayscale = 0.0f;
      float color = 1.0f;
      if (info.isKilled())
      {
         grayscale = std::min(1.0f, _grayscales[info.getColor()]);
         color -= grayscale;
      }

      const float left = layer.getLeft() + x;
      const float top = static_cast<float>(layer.getTop());
      const float width = static_cast<float>(layer.getWidth());

      font.setColor(1.0f, 1.0f, 1.0f, 1.0f);
      font.buildVertices(FONT_SCALE_NICK, info.getNick(), left, top + OFFSET_Y_PLAYER_NICK, width);
      font.draw();

      const PlayerStats& stats = info.getOverallStats();

      font.buildVertices(FONT_SCALE_WINS, std::to_string(stats.getWins()), left, top + layer.getHeight() + OFFSET_Y_PLAYER_WINS, width);
      font.draw();

      font.setColor(color * 0.9f + grayscale * 0.5f, color * 0.1f + grayscale * 0.5f, grayscale * 0.5f, 1.0f);
      font.buildVertices(FONT_SCALE_STATS, std::to_string(stats.getDeaths()), left + OFFSET_X_PLAYER_DEATHS, top + OFFSET_Y_PLAYER_DEATHS, width);
      font.draw();

      font.setColor(color * 0.9f + grayscale * 0.5f, color * 0.9f + grayscale * 0.5f, grayscale * 0.5f, 1.0f);
      font.buildVertices(FONT_SCALE_STATS, std::to_string(stats.getKills()), left + OFFSET_X_PLAYER_KILLS, top + OFFSET_Y_PLAYER_KILLS, width);
      font.draw();

      x -= layer.getWidth() + PLAYER_ICON_OFFSET;
   }

   font.setColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void GameStatsDrawable::drawGameTime()
{
   const int minutes = _time_left / 60;
   const std::string time = std::format("{:02}:{:02}", minutes, _time_left - minutes * 60);

   BitmapFont& font = *_time_font;
   font.buildVertices(FONT_SCALE_TIME, time, _game_time_offset_x, _game_time_offset_y, -1.0f, _game_time_height);
   font.draw();
}

void GameStatsDrawable::drawOrb()
{
   if (!_layer_panel_orb || !_layer_orb || _orb_palette.getWidth() == 0)
   {
      return;
   }

   _layer_panel_orb->get().render();

   // the orb runs through the palette as the game time passes
   const float duration = std::max(static_cast<float>(_duration), 1.0f);
   const int index = std::clamp(static_cast<int>(((_duration - _time_left) / duration) * 255.0f), 0, _orb_palette.getWidth() - 1);
   const uint32_t pixel = _orb_palette.getScanline(0)[index];

   activeDevice().setShader(_color_shader);
   activeDevice().setParameter(
      _param_color, Vector((pixel & 0xff) / 255.0f, ((pixel >> 8) & 0xff) / 255.0f, ((pixel >> 16) & 0xff) / 255.0f)
   );
   _layer_orb->get().render();
   activeDevice().setShader(getDefaultMenuShader());
}

void GameStatsDrawable::updateMuteIcons()
{
   if (_layer_mute_music)
   {
      _layer_mute_music->get().getLayer().setVisible(SoundManager::getInstance().isMusicMuted());
   }

   if (_layer_mute_sfx)
   {
      _layer_mute_sfx->get().getLayer().setVisible(SoundManager::getInstance().isSfxMuted());
   }
}

void GameStatsDrawable::drawSkull()
{
   if (_skull_alpha > 0.0f && _layers_skulls[_skull_type])
   {
      _layers_skulls[_skull_type]->get().render(0.0f, 0.0f, _skull_alpha);
   }
}

void GameStatsDrawable::drawSkullProgress()
{
   if (!_layer_skull_progress_middle || !_layer_skull_progress_start)
   {
      return;
   }

   const float progress = getSkullProgress();

   activeDevice().setShader(_progress_shader);
   activeDevice().setParameter(_param_progress, progress);
   _layer_skull_progress_middle->get().render();
   activeDevice().setShader(getDefaultMenuShader());

   const float offset = -(1.0f - progress) * _layer_skull_progress_middle->get().getWidth();
   _layer_skull_progress_start->get().render(offset);
}

float GameStatsDrawable::getSkullProgress() const
{
   if (_skull_aborted)
   {
      return 1.0f;
   }

   return (GlobalTime::Instance().getTime() - _skull_animation_start_time) / _skull_duration;
}

void GameStatsDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, _psd.getWidth(), _psd.getHeight(), 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice().setShader(getDefaultMenuShader());
}

void GameStatsDrawable::cleanupGlParameters()
{
   activeDevice().setShader(0);

   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void GameStatsDrawable::setGameTimeLeft(int time_left, int duration)
{
   _time_left = time_left;
   _duration = duration;
}

void GameStatsDrawable::resetGrayscales()
{
   if (GameStateMachine::getInstance().getState() == Constants::GamePreparing)
   {
      _grayscales.fill(0.0f);
   }
}

void GameStatsDrawable::setVisible(bool visible)
{
   Drawable::setVisible(visible);

   if (visible)
   {
      // start the clock at the full game duration until the server sends the time
      if (const auto info = BombermanClient::getInstance().getCurrentGameInformation())
      {
         const int duration = info->get().getDuration();
         setGameTimeLeft(duration, duration);
      }
   }
   else
   {
      _skull_alpha = 0.0f;
      _skull_aborted = true;
   }
}

void GameStatsDrawable::playerInfected(int id, Constants::SkullType skull_type, int /*infector_id*/, int /*extra_x*/, int /*extra_y*/)
{
   if (id != BombermanClient::getInstance().getPlayerId())
   {
      return;
   }

   _skull_aborted = (skull_type == Constants::SkullReset);

   if (!_skull_aborted)
   {
      _skull_alpha = 1.0f;
      _skull_type = skull_type;
      _skull_duration = 0.001f * SERVER_SKULL_DURATION;
      _skull_animation_start_time = GlobalTime::Instance().getTime();
   }
}
