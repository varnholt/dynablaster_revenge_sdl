#include "musicplayerdrawable.h"

#include "gldevice.h"

#include "framework/timerhandler.h"

#include "menus/bitmapfont.h"
#include "menus/defaultshader.h"
#include "menus/fontpool.h"
#include "menus/psdlayer.h"

#include "math/matrix.h"

#include "gamesettings.h"
#include "soundmanager.h"

#include "stringutils.h"
#include "tools/datapaths.h"

#include <cmath>

namespace
{
constexpr float ANIMATION_DELAY_TIME = 3000.0f;
constexpr float FADE_IN_TIME = 2000.0f;
constexpr float FADE_IDLE_TIME = 5000.0f;
constexpr float FADE_OUT_TIME = 2000.0f;

constexpr const char* LAYER_TRACK_LINE_1 = "track_name_line_1";
constexpr const char* LAYER_TRACK_LINE_2 = "track_name_line_2";
constexpr const char* LAYER_ARTIST = "artist_name";
constexpr const char* LAYER_ALBUM = "album_name";

constexpr float MENU_Y_FACTOR_INGAME = 0.865f;
constexpr float MENU_Y_FACTOR_MENUS = 0.865f;

constexpr int ALBUM_CORRECTION_OFFSET_X = 0;
constexpr int ALBUM_CORRECTION_OFFSET_Y = 5;
constexpr int ARTIST_CORRECTION_OFFSET_X = -2;
constexpr int ARTIST_CORRECTION_OFFSET_Y = 10;
constexpr int TRACK_LINE_1_CORRECTION_OFFSET_X = 0;
constexpr int TRACK_LINE_1_CORRECTION_OFFSET_Y = 2;
constexpr int TRACK_LINE_2_CORRECTION_OFFSET_X = 0;
constexpr int TRACK_LINE_2_CORRECTION_OFFSET_Y = 2;
}  // namespace

MusicPlayerDrawable::MusicPlayerDrawable(RenderDevice* dev) : Drawable(dev)
{
   _filename = "data/musicplayer/player.psd";
}

MusicPlayerDrawable::~MusicPlayerDrawable() = default;

void MusicPlayerDrawable::initializeGL()
{
   DataPaths::add("data/musicplayer");

   initializeLayers();

   _font = &FontPool::Instance().get("default")->get();

   DataPaths::remove("data/musicplayer");
}

void MusicPlayerDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   for (auto& layer : _psd.getLayers())
   {
      const std::string& layer_name = layer.getName();

      if (layer_name == LAYER_TRACK_LINE_1)
      {
         _font_offset_track_line1_x = layer.getLeft() + TRACK_LINE_1_CORRECTION_OFFSET_X;
         _font_offset_track_line1_y = layer.getTop() + TRACK_LINE_1_CORRECTION_OFFSET_Y;
         _font_offset_track_line1_height = layer.getHeight();
      }
      else if (layer_name == LAYER_TRACK_LINE_2)
      {
         _font_offset_track_line2_x = layer.getLeft() + TRACK_LINE_2_CORRECTION_OFFSET_X;
         _font_offset_track_line2_y = layer.getTop() + TRACK_LINE_2_CORRECTION_OFFSET_X;
         _font_offset_track_line2_height = layer.getHeight();
      }
      else if (layer_name == LAYER_ALBUM)
      {
         _font_offset_album_x = layer.getLeft() + ALBUM_CORRECTION_OFFSET_X;
         _font_offset_album_y = layer.getTop() + ALBUM_CORRECTION_OFFSET_Y;
         _font_offset_album_height = layer.getHeight();
      }
      else if (layer_name == LAYER_ARTIST)
      {
         _font_offset_artist_x = layer.getLeft() + ARTIST_CORRECTION_OFFSET_X;
         _font_offset_artist_y = layer.getTop() + ARTIST_CORRECTION_OFFSET_Y;
         _font_offset_artist_height = layer.getHeight();
      }

      if (layer.getWidth() + layer.getLeft() > _max_width)
         _max_width = layer.getWidth() + layer.getLeft();

      _psd_layers.push_back(std::make_unique<PSDLayer>(layer));
   }
}

void MusicPlayerDrawable::initGlParameters()
{
   static_cast<GLDevice*>(_device)->pushProjection();
   static_cast<GLDevice*>(_device)->setProjectionMatrix(
      Matrix::ortho(0.0f, (float)_psd.getWidth(), (float)_psd.getHeight(), 0.0f, -1.0f, 1.0f)
   );

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice->setShader(getDefaultMenuShader());
}

void MusicPlayerDrawable::cleanupGlParameters()
{
   activeDevice->setShader(0);

   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);

   static_cast<GLDevice*>(_device)->popProjection();
}

void MusicPlayerDrawable::paintGL()
{
   initGlParameters();

   const float x_offset = (1.0f - _animation_factor) * _max_width;
   const float y_factor = isInGame() ? MENU_Y_FACTOR_INGAME : MENU_Y_FACTOR_MENUS;

   for (size_t layer_index = 0; layer_index < _psd.getLayerCount(); layer_index++)
   {
      const PSD::Layer& psd_layer = _psd.getLayer(layer_index);

      if (psd_layer.isVisible())
      {
         const float top = -static_cast<float>(psd_layer.getTop()) * (1.0f - y_factor);
         _psd_layers[layer_index]->render(-x_offset, top);
      }
   }

   _font->setColor(1.0f, 1.0f, 1.0f, 0.75f);
   _font->buildVertices(
      0.12f, _artist.c_str(), (float)_font_offset_artist_x - x_offset, (float)_font_offset_artist_y * y_factor + _font_offset_artist_height
   );
   _font->draw();

   _font->setColor(1.0f, 1.0f, 1.0f, 0.5f);
   _font->buildVertices(
      0.1f, _album.c_str(), (float)_font_offset_album_x - x_offset, (float)_font_offset_album_y * y_factor + _font_offset_album_height
   );
   _font->draw();

   _font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
   _font->buildVertices(
      0.1f,
      _track_line1.c_str(),
      (float)_font_offset_track_line1_x - x_offset,
      (float)_font_offset_track_line1_y * y_factor + _font_offset_track_line1_height
   );
   _font->draw();

   _font->buildVertices(
      0.1f,
      _track_line2.c_str(),
      (float)_font_offset_track_line2_x - x_offset,
      (float)_font_offset_track_line2_y * y_factor + _font_offset_track_line2_height
   );
   _font->draw();

   cleanupGlParameters();
}

void MusicPlayerDrawable::animate(float /*time*/)
{
   const float msecs = FrameTimer::currentTime().msecsTo(_animation_stop_time);

   if (_fade_in)
   {
      if (msecs < 0.0f)
      {
         _fade_in = false;
         _idle = true;

         _animation_stop_time = FrameTimer::currentTime().addMSecs(FADE_IDLE_TIME);
         _animation_factor = 1.0f;
      }
      else
      {
         _animation_factor = 1.0f - (msecs / FADE_IN_TIME);
         _animation_factor = sinf(_animation_factor * 3.14159265f * 0.5f);
         _animation_factor = _animation_factor * _animation_factor * _animation_factor;
      }
   }
   else if (_idle)
   {
      if (msecs < 0.0f)
      {
         _idle = false;
         _fade_out = true;

         _animation_stop_time = FrameTimer::currentTime().addMSecs(FADE_OUT_TIME);
         _animation_factor = 1.0f;
      }
      else
      {
         _animation_factor = 1.0f;
      }
   }
   else if (_fade_out)
   {
      if (msecs < 0.0f)
      {
         _fade_out = false;
         _visible = false;
         _animating = false;
         _animation_factor = 0.0f;
      }
      else
      {
         _animation_factor = msecs / FADE_OUT_TIME;
         _animation_factor = sinf(_animation_factor * 3.14159265f * 0.5f);
      }
   }
}

void MusicPlayerDrawable::startAnimation()
{
   _animation_stop_time = FrameTimer::currentTime().addMSecs(FADE_IN_TIME);

   _fade_in = true;
   _visible = true;
}

void MusicPlayerDrawable::showCurrentlyPlaying(const std::string& artist, const std::string& album, const std::string& track)
{
   // matches the original's guard, minus isMusicMuted() - this port's SoundManager has no
   // separate mute flag, only a volume slider (see project memory on the F2/F3 shortcuts).
   if (SoundManager::getInstance().getVolumeMusic() <= 0.0f || !GameSettings::getInstance().getAudioSettings().isMusicPlayerVisibile())
      return;

   _artist = artist;
   _album = album;
   _track_line2.clear();
   _track_line1 = track;

   // break the track name onto a second line, moving one more trailing word at a time, until
   // line 1 fits - matches the original's own word-wrap loop exactly.
   int count = 0;
   while (_track_line1.length() > 20)
   {
      _track_line1 = track;
      _track_line2.clear();

      count++;

      std::vector<std::string> words = StringUtils::split(_track_line1, ' ');
      if (words.empty())
         break;

      for (int i = 0; i < count && !words.empty(); i++)
      {
         _track_line2 = words.back() + " " + _track_line2;
         words.pop_back();
      }

      _track_line2 = StringUtils::trim(_track_line2);

      _track_line1.clear();
      for (std::size_t i = 0; i < words.size(); i++)
      {
         if (i > 0)
            _track_line1 += " ";
         _track_line1 += words[i];
      }
   }

   if (!_animating)
   {
      _animating = true;
      TimerHandler::singleShot(ANIMATION_DELAY_TIME, [this]() { startAnimation(); });
   }
}

void MusicPlayerDrawable::setVisible(bool /*visible*/)
{
   _visible = true;
}

bool MusicPlayerDrawable::isInGame() const
{
   return _in_game;
}

void MusicPlayerDrawable::setInGame(bool game)
{
   _in_game = game;
}
