#pragma once

// GLES3 port of client/src/game/gamewindrawable.cpp.

#include "drawable.h"
#include "gamestatemachine.h"
#include "math/matrix.h"
#include "menus/psdlayer.h"
#include "render/texture.h"
#include "weighted.h"

#include <string>
#include <vector>

#include <cstdint>
#include <memory>
#include "math/color.h"

class BitmapFont;
class BlurFilter;
class FrameBuffer;
class GameInformation;
class Mesh;
class MotionMixer;
class PlayerMaterial;
class PlayerInfo;
class PlayerItem;
class SceneGraph;

class GameWinDrawable : public Drawable
{
public:
   GameWinDrawable(RenderDevice* dev, bool visible = false);
   ~GameWinDrawable() override;

   void initializeGL() override;
   void paintGL() override;

   void setVisible(bool visible) override;
   void animate(float time) override;

   void setColor(const Color& color);
   const Color& getColor();

   void setColorEnum(Constants::Color color);
   Constants::Color getColorEnum() const;

   void setWinnerName(const std::string& name);
   const std::string& getWinnerName();

   void setDrawGame(bool draw);
   bool isDrawGame() const;

protected:
   void stateChanged();
   void startWinAnimation();

private:
   void initGlParameters();
   void cleanupGlParameters();

   void drawWinnerText();
   void initGameData();
   void drawGameData();
   void drawLayers(float alpha);
   void initializeLayers();

   float getContentsAlpha() const;
   float getDrawableAlpha() const;
   float getRadius() const;

   void hideLayers();
   int computeScore(PlayerInfo* info) const;

   void initializeWinnerScene();
   void initializePlayerMaterial();
   void updateWinAnimation(float time, float dt);
   void playSound();

   void drawBackBuffer(float alpha);
   void drawScene();
   void drawSceneToFramebuffer(float alpha);
   void drawPsdContents(float alpha);

   Color _color;
   Constants::Color _color_enum = Constants::ColorWhite;
   std::string _winner_name;

   BitmapFont* _large_font = nullptr;
   BitmapFont* _default_font = nullptr;

   SceneGraph* _scene = nullptr;

   float _render_time = 0.0f;
   float _time = 0.0f;
   float _delta_time = 0.0f;
   float _start_time = 0.0f;

   bool _draw_game = false;

   std::unique_ptr<BlurFilter> _blur;

   // dedicated offscreen targets this port needs in place of the original's persistent
   // MainDrawable-owned game framebuffer (MainDrawable doesn't exist here - see gamedrawable.h).
   // _snapshot_texture is a plain (non-FBO-attached) copy of the just-rendered frame - BlurFilter
   // reads from it while writing into _backdrop_fb, avoiding a read/write feedback loop on the
   // same texture. _scene_fb holds the real 3D cup+player render, composited on top afterward.
   std::unique_ptr<FrameBuffer> _backdrop_fb;
   std::unique_ptr<FrameBuffer> _scene_fb;
   uint32_t _snapshot_texture = 0;

   GameInformation* _game_information = nullptr;
   std::vector<Weighted<PlayerInfo*, int>> _player_scores;
   float _player_scores_animated[10];

   PlayerItem* _player_item = nullptr;
   MotionMixer* _motion_mixer = nullptr;
   Mesh* _player_mesh = nullptr;
   PlayerMaterial* _player_material = nullptr;
   Texture _player_textures[10];

   std::string _filename;
   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;

   PSDLayer* _ranks[10];
   PSDLayer* _icons[10];
   PSDLayer* _names[10];
   PSDLayer* _points[10];
   PSDLayer* _bars[10];
};
