#pragma once

// GLES3 port of client/src/game/musicplayerdrawable.cpp.

#include "framework/drawable.h"
#include "framework/frametimer.h"
#include "image/psd.h"

#include <string>
#include <vector>

class BitmapFont;
class PSDLayer;

class MusicPlayerDrawable : public Drawable
{
public:
   MusicPlayerDrawable(RenderDevice*);
   ~MusicPlayerDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float time) override;

   void setVisible(bool visible) override;

   bool isInGame() const;
   void setInGame(bool game);

   //! mirrors the original's "musicPlaying" slot - shows a slide-in notification with the
   //! given artist/album/track, gated on a startup delay so it doesn't appear instantly.
   void showCurrentlyPlaying(const std::string& artist, const std::string& album, const std::string& track);

private:
   void initializeLayers();
   void startAnimation();

   void initGlParameters();
   void cleanupGlParameters();

   PSD mPsd;
   std::vector<PSDLayer*> mPsdLayers;
   std::string mFilename;

   BitmapFont* mFont;

   std::string mArtist;
   std::string mAlbum;
   std::string mTrackLine1;
   std::string mTrackLine2;

   FrameTimer mAnimationStopTime;

   float mAnimationFactor;

   bool mFadeIn;
   bool mIdle;
   bool mFadeOut;

   int mMaxWidth;

   int mFontOffsetArtistX;
   int mFontOffsetArtistY;
   int mFontOffsetArtistHeight;

   int mFontOffsetAlbumX;
   int mFontOffsetAlbumY;
   int mFontOffsetAlbumHeight;

   int mFontOffsetTrackLine1X;
   int mFontOffsetTrackLine1Y;
   int mFontOffsetTrackLine1Height;

   int mFontOffsetTrackLine2X;
   int mFontOffsetTrackLine2Y;
   int mFontOffsetTrackLine2Height;

   bool mAnimating;

   bool mInGame;
};
