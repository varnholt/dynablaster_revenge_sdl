#pragma once

// GLES3 port of client/src/game/gameplayernamedisplay.cpp.

#include "framework/frametimer.h"
#include "math/vector.h"
#include "render/texture.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class BitmapFont;
class PlayerItem;

class GamePlayerNameDisplay
{
public:
   GamePlayerNameDisplay();

   bool isActive() const;

   void setPlayerData(const std::map<int, std::unique_ptr<PlayerItem>>& players);
   void draw() const;
   void drawPlayTexts() const;
   void drawArrow() const;
   void start();
   void initialize();

private:
   void initGlParameters() const;
   void cleanupGlParameters() const;
   float computeFontAlpha() const;
   float computeArrowAlpha() const;

   FrameTimer _active_time;
   std::vector<Vector> _positions;
   std::vector<std::string> _names;
   std::optional<std::reference_wrapper<BitmapFont>> _font;
   bool _show_arrow = false;
   Texture _arrow_texture;
   Vector _arrow_position;
   uint32_t _arrow_shader = 0;
   uint32_t _arrow_vertex_buffer = 0;
   uint32_t _arrow_index_buffer = 0;
   int _arrow_param_texture = -1;
   int _arrow_param_alpha = -1;
};
