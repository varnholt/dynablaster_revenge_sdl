// GLES3 port of client/src/game/musicplayerdrawable.cpp.

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
#include "tools/filestream.h"

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

MusicPlayerDrawable::MusicPlayerDrawable(RenderDevice* dev)
    : Drawable(dev),
      mFont(nullptr),
      mAnimationFactor(0.0f),
      mFadeIn(false),
      mIdle(false),
      mFadeOut(false),
      mMaxWidth(-1),
      mFontOffsetArtistX(0),
      mFontOffsetArtistY(0),
      mFontOffsetArtistHeight(0),
      mFontOffsetAlbumX(0),
      mFontOffsetAlbumY(0),
      mFontOffsetAlbumHeight(0),
      mFontOffsetTrackLine1X(0),
      mFontOffsetTrackLine1Y(0),
      mFontOffsetTrackLine1Height(0),
      mFontOffsetTrackLine2X(0),
      mFontOffsetTrackLine2Y(0),
      mFontOffsetTrackLine2Height(0),
      mAnimating(false),
      mInGame(false)
{
   mFilename = "data/musicplayer/player.psd";
}

MusicPlayerDrawable::~MusicPlayerDrawable()
{
   for (PSDLayer* layer : mPsdLayers)
      delete layer;
   mPsdLayers.clear();
}

void MusicPlayerDrawable::initializeGL()
{
   FileStream::addPath("data/musicplayer");

   initializeLayers();

   mFont = FontPool::Instance()->get("default");

   FileStream::removePath("data/musicplayer");
}

void MusicPlayerDrawable::initializeLayers()
{
   mPsd.load(mFilename.c_str());

   for (int l = 0; l < mPsd.getLayerCount(); l++)
   {
      PSD::Layer* layer = mPsd.getLayer(l);

      const std::string layerName = layer->getName();

      if (layerName == LAYER_TRACK_LINE_1)
      {
         mFontOffsetTrackLine1X = layer->getLeft() + TRACK_LINE_1_CORRECTION_OFFSET_X;
         mFontOffsetTrackLine1Y = layer->getTop() + TRACK_LINE_1_CORRECTION_OFFSET_Y;
         mFontOffsetTrackLine1Height = layer->getHeight();
      }
      else if (layerName == LAYER_TRACK_LINE_2)
      {
         mFontOffsetTrackLine2X = layer->getLeft() + TRACK_LINE_2_CORRECTION_OFFSET_X;
         mFontOffsetTrackLine2Y = layer->getTop() + TRACK_LINE_2_CORRECTION_OFFSET_X;
         mFontOffsetTrackLine2Height = layer->getHeight();
      }
      else if (layerName == LAYER_ALBUM)
      {
         mFontOffsetAlbumX = layer->getLeft() + ALBUM_CORRECTION_OFFSET_X;
         mFontOffsetAlbumY = layer->getTop() + ALBUM_CORRECTION_OFFSET_Y;
         mFontOffsetAlbumHeight = layer->getHeight();
      }
      else if (layerName == LAYER_ARTIST)
      {
         mFontOffsetArtistX = layer->getLeft() + ARTIST_CORRECTION_OFFSET_X;
         mFontOffsetArtistY = layer->getTop() + ARTIST_CORRECTION_OFFSET_Y;
         mFontOffsetArtistHeight = layer->getHeight();
      }

      if (layer->getWidth() + layer->getLeft() > mMaxWidth)
         mMaxWidth = layer->getWidth() + layer->getLeft();

      mPsdLayers.push_back(new PSDLayer(layer));
   }
}

void MusicPlayerDrawable::initGlParameters()
{
   static_cast<GLDevice*>(mDevice)->pushProjection();
   static_cast<GLDevice*>(mDevice)->setProjectionMatrix(
      Matrix::ortho(0.0f, (float)mPsd.getWidth(), (float)mPsd.getHeight(), 0.0f, -1.0f, 1.0f)
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

   static_cast<GLDevice*>(mDevice)->popProjection();
}

void MusicPlayerDrawable::paintGL()
{
   initGlParameters();

   const float xOffset = (1.0f - mAnimationFactor) * mMaxWidth;
   const float yFactor = isInGame() ? MENU_Y_FACTOR_INGAME : MENU_Y_FACTOR_MENUS;

   for (int layerIndex = 0; layerIndex < mPsd.getLayerCount(); layerIndex++)
   {
      PSD::Layer* psdLayer = mPsd.getLayer(layerIndex);

      if (psdLayer->isVisible())
      {
         const float top = -(float)psdLayer->getTop() * (1.0f - yFactor);
         mPsdLayers[layerIndex]->render(-xOffset, top);
      }
   }

   mFont->setColor(1.0f, 1.0f, 1.0f, 0.75f);
   mFont->buildVertices(
      0.12f, mArtist.c_str(), (float)mFontOffsetArtistX - xOffset, (float)mFontOffsetArtistY * yFactor + mFontOffsetArtistHeight
   );
   mFont->draw();

   mFont->setColor(1.0f, 1.0f, 1.0f, 0.5f);
   mFont->buildVertices(
      0.1f, mAlbum.c_str(), (float)mFontOffsetAlbumX - xOffset, (float)mFontOffsetAlbumY * yFactor + mFontOffsetAlbumHeight
   );
   mFont->draw();

   mFont->setColor(1.0f, 1.0f, 1.0f, 1.0f);
   mFont->buildVertices(
      0.1f,
      mTrackLine1.c_str(),
      (float)mFontOffsetTrackLine1X - xOffset,
      (float)mFontOffsetTrackLine1Y * yFactor + mFontOffsetTrackLine1Height
   );
   mFont->draw();

   mFont->buildVertices(
      0.1f,
      mTrackLine2.c_str(),
      (float)mFontOffsetTrackLine2X - xOffset,
      (float)mFontOffsetTrackLine2Y * yFactor + mFontOffsetTrackLine2Height
   );
   mFont->draw();

   cleanupGlParameters();
}

void MusicPlayerDrawable::animate(float /*time*/)
{
   const float msecs = FrameTimer::currentTime().msecsTo(mAnimationStopTime);

   if (mFadeIn)
   {
      if (msecs < 0.0f)
      {
         mFadeIn = false;
         mIdle = true;

         mAnimationStopTime = FrameTimer::currentTime().addMSecs(FADE_IDLE_TIME);
         mAnimationFactor = 1.0f;
      }
      else
      {
         mAnimationFactor = 1.0f - (msecs / FADE_IN_TIME);
         mAnimationFactor = sinf(mAnimationFactor * 3.14159265f * 0.5f);
         mAnimationFactor = mAnimationFactor * mAnimationFactor * mAnimationFactor;
      }
   }
   else if (mIdle)
   {
      if (msecs < 0.0f)
      {
         mIdle = false;
         mFadeOut = true;

         mAnimationStopTime = FrameTimer::currentTime().addMSecs(FADE_OUT_TIME);
         mAnimationFactor = 1.0f;
      }
      else
      {
         mAnimationFactor = 1.0f;
      }
   }
   else if (mFadeOut)
   {
      if (msecs < 0.0f)
      {
         mFadeOut = false;
         mVisible = false;
         mAnimating = false;
         mAnimationFactor = 0.0f;
      }
      else
      {
         mAnimationFactor = msecs / FADE_OUT_TIME;
         mAnimationFactor = sinf(mAnimationFactor * 3.14159265f * 0.5f);
      }
   }
}

void MusicPlayerDrawable::startAnimation()
{
   mAnimationStopTime = FrameTimer::currentTime().addMSecs(FADE_IN_TIME);

   mFadeIn = true;
   mVisible = true;
}

void MusicPlayerDrawable::showCurrentlyPlaying(const std::string& artist, const std::string& album, const std::string& track)
{
   // matches the original's guard, minus isMusicMuted() - this port's SoundManager has no
   // separate mute flag, only a volume slider (see project memory on the F2/F3 shortcuts).
   if (SoundManager::getInstance()->getVolumeMusic() <= 0.0f || !GameSettings::getInstance()->getAudioSettings()->isMusicPlayerVisibile())
      return;

   mArtist = artist;
   mAlbum = album;
   mTrackLine2.clear();
   mTrackLine1 = track;

   // break the track name onto a second line, moving one more trailing word at a time, until
   // line 1 fits - matches the original's own word-wrap loop exactly.
   int count = 0;
   while (mTrackLine1.length() > 20)
   {
      mTrackLine1 = track;
      mTrackLine2.clear();

      count++;

      std::vector<std::string> words = StringUtils::split(mTrackLine1, ' ');
      if (words.empty())
         break;

      for (int i = 0; i < count && !words.empty(); i++)
      {
         mTrackLine2 = words.back() + " " + mTrackLine2;
         words.pop_back();
      }

      mTrackLine2 = StringUtils::trim(mTrackLine2);

      mTrackLine1.clear();
      for (std::size_t i = 0; i < words.size(); i++)
      {
         if (i > 0)
            mTrackLine1 += " ";
         mTrackLine1 += words[i];
      }
   }

   if (!mAnimating)
   {
      mAnimating = true;
      TimerHandler::singleShot(ANIMATION_DELAY_TIME, [this]() { startAnimation(); });
   }
}

void MusicPlayerDrawable::setVisible(bool /*visible*/)
{
   mVisible = true;
}

bool MusicPlayerDrawable::isInGame() const
{
   return mInGame;
}

void MusicPlayerDrawable::setInGame(bool game)
{
   mInGame = game;
}
