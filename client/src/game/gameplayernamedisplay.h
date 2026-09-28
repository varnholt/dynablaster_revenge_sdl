#pragma once

// GLES3 port of client/src/game/gameplayernamedisplay.cpp.

#include "framework/frametimer.h"
#include "math/vector.h"
#include "render/texture.h"

#include <map>
#include <string>
#include <vector>

class BitmapFont;
class PlayerItem;

class GamePlayerNameDisplay
{
public:
   GamePlayerNameDisplay();

   bool isActive() const;

   void setPlayerData(std::map<int, PlayerItem*>& players);
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
   BitmapFont* _font;
   bool _show_arrow;
   Texture _arrow_texture;
   Vector _arrow_position;
   unsigned int _arrow_shader;
   unsigned int _arrow_vertex_buffer;
   unsigned int _arrow_index_buffer;
   int _arrow_param_texture;
   int _arrow_param_alpha;
};
