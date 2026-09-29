#pragma once

#include "framework/drawable.h"
#include "framework/frametimer.h"
#include "image/psd.h"

#include <memory>
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

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;
   std::string _filename;

   BitmapFont* _font = nullptr;

   std::string _artist;
   std::string _album;
   std::string _track_line1;
   std::string _track_line2;

   FrameTimer _animation_stop_time;

   float _animation_factor = 0.0f;

   bool _fade_in = false;
   bool _idle = false;
   bool _fade_out = false;

   int _max_width = -1;

   int _font_offset_artist_x = 0;
   int _font_offset_artist_y = 0;
   int _font_offset_artist_height = 0;

   int _font_offset_album_x = 0;
   int _font_offset_album_y = 0;
   int _font_offset_album_height = 0;

   int _font_offset_track_line1_x = 0;
   int _font_offset_track_line1_y = 0;
   int _font_offset_track_line1_height = 0;

   int _font_offset_track_line2_x = 0;
   int _font_offset_track_line2_y = 0;
   int _font_offset_track_line2_height = 0;

   bool _animating = false;

   bool _in_game = false;
};
