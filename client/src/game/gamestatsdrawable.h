#pragma once

#include "drawable.h"

#include "constants.h"
#include "image/image.h"
#include "image/psd.h"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class BitmapFont;
class PSDLayer;

/// \brief the in-game HUD: game clock with its colour-cycling orb and the local player's skull
/// countdown (top left), every player's icon with nick, wins, kills and deaths (top right) and the
/// mute icons (bottom right). Layout comes from data/game/gameui.psd.
class GameStatsDrawable : public Drawable
{
public:
   explicit GameStatsDrawable(RenderDevice& dev);
   ~GameStatsDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float global_time) override;
   void setVisible(bool visible) override;

   void setGameTimeLeft(int time_left, int duration);
   void playerInfected(int id, Constants::SkullType skull_type, int infector_id, int extra_x, int extra_y);

   //! dead players fade to grey again from the next game on
   void resetGrayscales();

protected:
   void initializeLayers();
   void initGlParameters();
   void cleanupGlParameters();

   void drawOverlay();
   void drawPlayerIcons();
   void drawPlayerInfo();
   void drawGameTime();
   void drawOrb();
   void drawSkull();
   void drawSkullProgress();
   void updateMuteIcons();

   void animateDeathGrayScales(float dt);
   void animateSkulls(float dt);
   float getSkullProgress() const;

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;
   std::string _filename;

   std::optional<std::reference_wrapper<BitmapFont>> _time_font;
   std::optional<std::reference_wrapper<BitmapFont>> _outline_font;

   Image _orb_palette;

   std::array<std::optional<std::reference_wrapper<PSDLayer>>, 10> _layers_player;
   std::array<std::optional<std::reference_wrapper<PSDLayer>>, Constants::SkullReset + 1> _layers_skulls;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_orb;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_panel_orb;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_mute_music;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_mute_sfx;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_skull_progress_start;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_skull_progress_middle;

   int _time_left = 0;
   int _duration = 0;

   float _game_time_offset_x = 0.0f;
   float _game_time_offset_y = 0.0f;
   float _game_time_height = 0.0f;

   // dead players' icons fade to grey, indexed by Constants::Color
   uint32_t _grayscale_shader = 0;
   int _param_grayscale = -1;
   std::array<float, 11> _grayscales{};

   uint32_t _color_shader = 0;
   int _param_color = -1;

   float _last_global_time = 0.0f;

   Constants::SkullType _skull_type = Constants::SkullReset;
   float _skull_duration = 0.0f;
   float _skull_animation_start_time = 0.0f;
   float _skull_alpha = 0.0f;
   bool _skull_aborted = true;

   uint32_t _progress_shader = 0;
   int _param_progress = -1;
};
