// GLES3 port of client/src/game/gamewindrawable.cpp.

#include "gamewindrawable.h"

#include "framework/framebuffer.h"
#include "gldevice.h"
#include "postproduction/blurfilter.h"

#include "tools/datapaths.h"

#include "bombermanclient.h"
#include "gamesettings.h"
#include "menus/defaultshader.h"
#include "menus/fontpool.h"
#include "playeritem.h"
#include "soundmanager.h"

#include "engine/animation/motionmixer.h"
#include "materials/environmentambientdiffusematerial.h"
#include "materials/environmentambientmaterial.h"
#include "materials/material.h"
#include "materials/materialfactory.h"
#include "materials/playermaterial.h"
#include "nodes/camera.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "render/texturepool.h"
#include "timer.h"

#include "logging.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <map>
#include <numbers>
#include <string>

#define OFFSET_X_POINTS -43
#define OFFSET_Y_POINTS -2
#define OFFSET_Y_NAME -2
#define OFFSET_Y_ICON -20
#define CENTER_WIDTH_SCORE 130
#define RADIUS_MAX 200.0f

namespace
{
class CupMaterialFactory : public MaterialFactory
{
public:
   Material* createMaterial(SceneGraph* scene, int id) const override
   {
      Material* mat = nullptr;
      switch (id)
      {
         case (MAP_AMBIENT | MAP_REFLECT):
            mat = new EnvironmentAmbientMaterial(scene);
            break;
         case (MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT):
            mat = new EnvironmentAmbientDiffuseMaterial(scene);
            break;
         default:
            break;
      }
      return mat;
   }
};
}  // namespace

GameWinDrawable::GameWinDrawable(RenderDevice* dev, bool visible) : Drawable(dev, visible)
{
   _filename = "data/game/results.psd";

   GameStateMachine::getInstance()->stateChangedSignal.connect([this]() { stateChanged(); });
}

GameWinDrawable::~GameWinDrawable()
{
   // deletes _player_mesh too
   delete _player_item;
   delete _motion_mixer;
   delete _player_material;
}

void GameWinDrawable::initializeGL()
{
   DataPaths::add("data/cup");

   _large_font = &FontPool::Instance().get("large")->get();
   _default_font = &FontPool::Instance().get("large-outlined")->get();

   _scene = new SceneGraph();
   CupMaterialFactory factory;
   _scene->load("cup.hjb", &factory);

   Node* node = _scene->getNode("Cup");
   node->setUserTransformable(true);

   _scene->getCamera()->setUserTransformable(true);

   _scene->render();

   DataPaths::remove("data/cup");

   _blur = std::make_unique<BlurFilter>();
   _blur->init();

   glGenTextures(1, &_snapshot_texture);

   initializeLayers();

   initializePlayerMaterial();
   initializeWinnerScene();
}

float GameWinDrawable::getContentsAlpha() const
{
   float alpha = 0.0f;
   float elapsed = _time;

   if (elapsed < SHOW_WINNER_FADE_IN_TIME)
   {
      alpha = elapsed / static_cast<float>(SHOW_WINNER_FADE_IN_TIME);
   }
   else if (elapsed > (SHOW_WINNER_FADE_IN_TIME + SHOW_WINNER_DISPLAY_TIME + SHOW_WINNER_ADDITIONAL_TIME))
   {
      alpha = std::max(
         1.0f - ((elapsed - SHOW_WINNER_FADE_IN_TIME - SHOW_WINNER_DISPLAY_TIME - SHOW_WINNER_ADDITIONAL_TIME) /
                 static_cast<float>(SHOW_WINNER_FADE_OUT_TIME)),
         0.0f
      );
   }
   else
   {
      alpha = 1.0f;
   }

   return alpha;
}

float GameWinDrawable::getDrawableAlpha() const
{
   float alpha = 1.0f;
   float elapsed = _time;

   float duration = SHOW_WINNER_FADE_IN_TIME + SHOW_WINNER_DISPLAY_TIME + SHOW_WINNER_ADDITIONAL_TIME + SHOW_WINNER_FADE_OUT_TIME;

   if (elapsed > duration)
   {
      alpha = std::max(1.0f - ((elapsed - duration) / static_cast<float>(SHOW_WINNER_SHOW_MENU_TIME)), 0.0f);
   }

   return alpha;
}

void GameWinDrawable::hideLayers()
{
   for (int i = 0; i < 10; i++)
   {
      _ranks[i]->getLayer()->setVisible(false);
      _icons[i]->getLayer()->setVisible(false);
      _names[i]->getLayer()->setVisible(false);
      _points[i]->getLayer()->setVisible(false);
      _bars[i]->getLayer()->setVisible(false);
   }
}

void GameWinDrawable::drawWinnerText()
{
   float alpha = getContentsAlpha();
   float y_offset = std::sin(_render_time * 0.05f) * 1080.0f * 0.015f;

   const float font_size = 0.7f;

   _large_font->setColor(1.0f, 1.0f, 1.0f, alpha);

   if (isDrawGame())
   {
      _large_font->buildVertices(font_size, "draw game", 0.0f, 0.15f * 1080.0f + y_offset, 1920.0f);
      _large_font->draw();
   }
   else
   {
      std::string win_text = std::format("{} wins!", getWinnerName());
      _large_font->buildVertices(font_size, win_text.c_str(), 0.0f, 0.15f * 1080.0f + y_offset, 1920.0f);
      _large_font->draw();
   }
}

void GameWinDrawable::initGameData()
{
   _game_information = BombermanClient::getInstance()->getCurrentGameInformation();

   _player_scores.clear();
   std::vector<PlayerInfo*> info_list = BombermanClient::getInstance()->getPlayerInfoList();

   for (PlayerInfo* info : info_list)
   {
      Weighted<PlayerInfo*, int> w(info, computeScore(info));
      _player_scores.push_back(w);
   }

   std::sort(_player_scores.begin(), _player_scores.end());

   hideLayers();

   int row = 0;
   for (const Weighted<PlayerInfo*, int>& w : _player_scores)
   {
      PlayerInfo* info = w.getObject();
      Constants::Color color = info->getColor();
      int color_index = color - 1;

      qDebug("GameWinDrawable::initGameData(): #%d: %s", w.getWeight(), info->getNick().c_str());

      PSDLayer* rank_layer = _ranks[row];
      PSDLayer* name_layer = _names[row];
      PSDLayer* bar_layer = _bars[row];
      PSDLayer* icon_layer = _icons[color_index];

      icon_layer->getLayer()->setY(name_layer->getTop() + OFFSET_Y_ICON);

      rank_layer->getLayer()->setVisible(true);
      bar_layer->getLayer()->setVisible(true);
      icon_layer->getLayer()->setVisible(true);

      row++;
   }

   std::memset(_player_scores_animated, 0, 10 * sizeof(float));
}

int GameWinDrawable::computeScore(PlayerInfo* info) const
{
   PlayerStats ps = info->getRoundStats();

   float score = 500.0f * ps.getWins() + 10.0f * ps.getKills() + 5.0f * ps.getExtrasCollected() + 0.25f * ps.getSurvivalTime();

   score *= 0.1f;

   return static_cast<int32_t>(score);
}

void GameWinDrawable::drawGameData()
{
   int row = 0;
   for (const Weighted<PlayerInfo*, int>& w : _player_scores)
   {
      PlayerInfo* info = w.getObject();

      PSDLayer* name_layer = _names[row];
      PSDLayer* points_layer = _points[row];

      _default_font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
      _default_font->buildVertices(0.27f, info->getNick().c_str(), name_layer->getLeft(), name_layer->getBottom() + OFFSET_Y_NAME);
      _default_font->draw();

      const int color_index = static_cast<int32_t>(info->getColor()) - 1;
      _player_scores_animated[color_index] += _delta_time;
      const int score = static_cast<int32_t>(std::min(_player_scores_animated[color_index], static_cast<float>(computeScore(info))));

      const auto score_text = std::to_string(score);
      _default_font->buildVertices(
         0.27f,
         score_text.c_str(),
         points_layer->getLeft() + OFFSET_X_POINTS,
         points_layer->getBottom() + OFFSET_Y_POINTS,
         CENTER_WIDTH_SCORE
      );
      _default_font->draw();

      row++;
   }
}

void GameWinDrawable::drawLayers(float alpha)
{
   for (size_t layer_index = 0; layer_index < _psd.getLayerCount(); layer_index++)
   {
      if (_psd.getLayer(layer_index).isVisible())
      {
         _psd_layers[layer_index]->render(0.0f, 0.0f, alpha);
      }
   }
}

float GameWinDrawable::getRadius() const
{
   float radius = _render_time * 0.5f;

   if (radius > RADIUS_MAX)
      radius = RADIUS_MAX;

   float fade_out_start =
      static_cast<float>(SHOW_WINNER_FADE_IN_TIME + SHOW_WINNER_DISPLAY_TIME + SHOW_WINNER_ADDITIONAL_TIME + SHOW_WINNER_FADE_OUT_TIME);

   if (_time > fade_out_start)
   {
      radius = std::max(1.0f - ((_time - fade_out_start) / static_cast<float>(SHOW_WINNER_SHOW_MENU_TIME)), 0.0f);
      radius *= RADIUS_MAX;
   }

   return radius;
}

void GameWinDrawable::drawBackBuffer(float alpha)
{
   const int width = activeDevice->getWidth();
   const int height = activeDevice->getHeight();

   // snapshot the just-rendered frame into a plain (non-FBO-attached) texture - BlurFilter reads
   // from this while writing its result into _backdrop_fb below; sampling and writing the same
   // live FBO attachment at once would be an undefined feedback loop.
   glBindTexture(GL_TEXTURE_2D, _snapshot_texture);
   FrameBuffer::copyTexImage(0, 0, width, height);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   if (!_backdrop_fb)
      _backdrop_fb = std::make_unique<FrameBuffer>(width, height, 0, FrameBuffer::NoDepthBuffer);
   else
      _backdrop_fb->setResolution(width, height);

   const float radius = getRadius();

   _backdrop_fb->bind();
   _blur->setAlpha(1.0f);
   _blur->setRadius(radius);
   _blur->process(_snapshot_texture, 1, 1);
   _backdrop_fb->unbind();

   // composite the blurred backdrop onto the real screen - whole-rect fade, not per-pixel alpha
   // (matches getFramebufferBlitShader()'s documented "replace alpha" use case).
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(getFramebufferBlitShader());
   static_cast<GLDevice*>(activeDevice)->setProjectionMatrix(Matrix());
   activeDevice->push(Matrix());
   activeDevice->setParameter(getFramebufferBlitShaderAlphaParam(), alpha);
   _backdrop_fb->draw(alpha);
   activeDevice->pop();
   activeDevice->setShader(0);

   glDisable(GL_BLEND);
}

void GameWinDrawable::drawScene()
{
   if (!isDrawGame())
   {
      // every material which has not been used yet restores its own textures the first time it's
      // used - so after setting a different color map earlier, it reverts to the one it was
      // created with. resetting the color map after we know the material has been used is the
      // simplest workaround.
      _player_material->setColorMap(_player_textures[getColorEnum() - 1]);

      // after the fov was fixed to match the 3dsmax settings, it's too narrow in this scene -
      // compensate manually.
      Matrix scale = Matrix::scale(0.75f, 0.75f, 1.0f);
      _scene->render(_render_time, scale);
   }
}

void GameWinDrawable::drawSceneToFramebuffer(float alpha)
{
   const int width = activeDevice->getWidth();
   const int height = activeDevice->getHeight();

   if (!_scene_fb)
      _scene_fb = std::make_unique<FrameBuffer>(width, height, 0, 0);
   else
      _scene_fb->setResolution(width, height);

   _scene_fb->bind();
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   drawScene();
   _scene_fb->unbind();

   // composite on top of the already-drawn blurred backdrop - real per-pixel alpha this time
   // (transparent gaps around the cup/player let the backdrop underneath show through).
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(getDefaultMenuShader());
   static_cast<GLDevice*>(activeDevice)->setProjectionMatrix(Matrix());
   activeDevice->push(Matrix());
   activeDevice->setParameter(getDefaultMenuShaderAlphaParam(), alpha);
   _scene_fb->draw(alpha);
   activeDevice->pop();
   activeDevice->setShader(0);

   glDisable(GL_BLEND);
}

void GameWinDrawable::drawPsdContents(float alpha)
{
   initGlParameters();
   drawWinnerText();
   drawLayers(alpha);
   drawGameData();
   cleanupGlParameters();
}

void GameWinDrawable::paintGL()
{
   float alpha = getDrawableAlpha();

   drawBackBuffer(alpha);
   drawSceneToFramebuffer(alpha);
   drawPsdContents(alpha);

   if (alpha == 0.0f)
      Drawable::setVisible(false);
}

void GameWinDrawable::setVisible(bool visible)
{
   if (visible)
      _start_time = 0.0f;

   Drawable::setVisible(visible);
}

void GameWinDrawable::animate(float time)
{
   if (_start_time == 0.0f)
      _start_time = time;

   float time_offset = time - _start_time;

   _delta_time = time_offset - _render_time;
   _render_time = time_offset;
   _time = _render_time * 16.0f;

   if (_delta_time < 0.0f)
      _delta_time = 0.0f;

   _player_item->animate(_render_time, _delta_time);

   Matrix cup_transform = _scene->getNode("Cup")->getTransform();

   // move player elsewhere
   cup_transform.xw = 200.0f;
   cup_transform.yw = 0.0f;
   cup_transform.zw = 50.0f;
   cup_transform.ww = 1.0f;
   Matrix rotzc = Matrix::rotateZ(0.005f * std::sin(0.01f * time));
   Matrix rotyc = Matrix::rotateY(0.001f * std::sin(0.01f * time));
   _scene->getNode("Cup")->setTransform(rotyc * rotzc * cup_transform);

   float scale = 365;
   Matrix player_matrix = Matrix::scale(scale, scale, scale);
   player_matrix.xw = 345.0f;
   player_matrix.yw = 0.0f;
   player_matrix.zw = -40.0f;
   player_matrix.ww = 1.0f;

   Matrix rotz = Matrix::rotateZ(std::numbers::pi_v<float>);
   Matrix rotx = Matrix::rotateY(0.11f);
   player_matrix = player_matrix * rotz * rotx;

   _player_mesh->setUserTransformable(true);
   _player_mesh->setTransform(player_matrix);
}

void GameWinDrawable::setColor(const Color& color)
{
   _color = color;
}

const Color& GameWinDrawable::getColor()
{
   return _color;
}

void GameWinDrawable::setColorEnum(Constants::Color color)
{
   _color_enum = color;
}

Constants::Color GameWinDrawable::getColorEnum() const
{
   return _color_enum;
}

void GameWinDrawable::setWinnerName(const std::string& name)
{
   _winner_name = name;
}

const std::string& GameWinDrawable::getWinnerName()
{
   return _winner_name;
}

void GameWinDrawable::setDrawGame(bool draw)
{
   _draw_game = draw;
}

bool GameWinDrawable::isDrawGame() const
{
   return _draw_game;
}

void GameWinDrawable::playSound()
{
   if (isDrawGame())
      Timer::singleShot(500, []() { SoundManager::getInstance()->playSoundGameDraw(); });
   else
      Timer::singleShot(500, []() { SoundManager::getInstance()->playSoundGameWin(); });
}

void GameWinDrawable::stateChanged()
{
   if (GameStateMachine::getInstance()->getState() == Constants::GameStopped)
   {
      // if the player pressed ESC, there's no valid game id anymore - and no results screen to show.
      if (BombermanClient::getInstance()->isGameIdValid())
      {
         SoundManager::getInstance()->fadeOut(1000);

         std::vector<PlayerInfo*> player_alive;
         std::vector<PlayerInfo*> player_list = BombermanClient::getInstance()->getPlayerInfoList();

         for (PlayerInfo* player : player_list)
         {
            if (!player->isKilled())
               player_alive.push_back(player);
         }

         if (player_alive.size() == 1)
         {
            setWinnerName(player_alive[0]->getNick());

            Constants::Color color = player_alive[0]->getColor();
            setColorEnum(color);
            setColor(GameSettings::getInstance()->getStyleSettings()->getColor(color));
         }

         setDrawGame(player_alive.size() != 1);

         initGameData();

         Timer::singleShot(500, [this]() { startWinAnimation(); });

         playSound();

         setVisible(true);
      }
   }
}

void GameWinDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0, 1920, 1080, 0, -1.0f, 1.0f);
   static_cast<GLDevice*>(activeDevice)->setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice->setShader(0);
}

void GameWinDrawable::cleanupGlParameters()
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void GameWinDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   std::map<std::string, int> ranks;
   ranks.insert({"p1-rank", 0});
   ranks.insert({"p2-rank", 1});
   ranks.insert({"p3-rank", 2});
   ranks.insert({"p4-rank", 3});
   ranks.insert({"p5-rank", 4});
   ranks.insert({"p6-rank", 5});
   ranks.insert({"p7-rank", 6});
   ranks.insert({"p8-rank", 7});
   ranks.insert({"p9-rank", 8});
   ranks.insert({"p10-rank", 9});

   std::map<std::string, Constants::Color> icons;
   icons.insert({"white-icon", Constants::ColorWhite});
   icons.insert({"black-icon", Constants::ColorBlack});
   icons.insert({"red-icon", Constants::ColorRed});
   icons.insert({"green-icon", Constants::ColorGreen});
   icons.insert({"blue-icon", Constants::ColorBlue});
   icons.insert({"silver-icon", Constants::ColorGrey});
   icons.insert({"gold-icon", Constants::ColorYellow});
   icons.insert({"purple-icon", Constants::ColorPurple});
   icons.insert({"cyan-icon", Constants::ColorCyan});
   icons.insert({"orange-icon", Constants::ColorOrange});

   std::map<std::string, int> names;
   names.insert({"p1-name", 0});
   names.insert({"p2-name", 1});
   names.insert({"p3-name", 2});
   names.insert({"p4-name", 3});
   names.insert({"p5-name", 4});
   names.insert({"p6-name", 5});
   names.insert({"p7-name", 6});
   names.insert({"p8-name", 7});
   names.insert({"p9-name", 8});
   names.insert({"p10-name", 9});

   std::map<std::string, int> points;
   points.insert({"p1-points", 0});
   points.insert({"p2-points", 1});
   points.insert({"p3-points", 2});
   points.insert({"p4-points", 3});
   points.insert({"p5-points", 4});
   points.insert({"p6-points", 5});
   points.insert({"p7-points", 6});
   points.insert({"p8-points", 7});
   points.insert({"p9-points", 8});
   points.insert({"p10-points", 9});

   std::map<std::string, int> bars;
   bars.insert({"bar-bg-1", 0});
   bars.insert({"bar-bg-2", 1});
   bars.insert({"bar-bg-3", 2});
   bars.insert({"bar-bg-4", 3});
   bars.insert({"bar-bg-5", 4});
   bars.insert({"bar-bg-6", 5});
   bars.insert({"bar-bg-7", 6});
   bars.insert({"bar-bg-8", 7});
   bars.insert({"bar-bg-9", 8});
   bars.insert({"bar-bg-10", 9});

   for (auto& psd_layer : _psd.getLayers())
   {
      auto owned_layer = std::make_unique<PSDLayer>(psd_layer);
      PSDLayer* layer = owned_layer.get();
      std::string layer_name = layer->getLayer()->getName();

      _psd_layers.push_back(std::move(owned_layer));

      auto rank_iterator = ranks.find(layer_name);
      if (rank_iterator != ranks.end())
         _ranks[rank_iterator->second] = layer;

      auto icons_iterator = icons.find(layer_name);
      if (icons_iterator != icons.end())
         _icons[icons_iterator->second - 1] = layer;

      auto names_iterator = names.find(layer_name);
      if (names_iterator != names.end())
         _names[names_iterator->second] = layer;

      auto points_iterator = points.find(layer_name);
      if (points_iterator != points.end())
         _points[points_iterator->second] = layer;

      auto bars_iterator = bars.find(layer_name);
      if (bars_iterator != bars.end())
         _bars[bars_iterator->second] = layer;
   }
}

void GameWinDrawable::initializeWinnerScene()
{
   Mesh* mesh = MotionMixer::getMesh("bomberman");
   mesh->setUserTransformable(true);

   _player_mesh = new Mesh(_scene);
   _player_mesh->copy(*mesh);

   _motion_mixer = new MotionMixer();
   _player_mesh->setMotionMixer(_motion_mixer);
   _player_mesh->setVisible(true);
   _player_mesh->setUserTransformable(true);

   _player_material->addMesh(_player_mesh);

   _player_item = new PlayerItem(0, "winner", Constants::ColorCyan);
   _player_item->setMesh(_player_mesh);
   _player_item->setPosition(0.0f, 0.0f);
   _player_item->setKilled(false);
}

void GameWinDrawable::initializePlayerMaterial()
{
   delete _player_material;
   _player_material = nullptr;

   DataPaths::add("data/winner");

   const auto material_texture_name = std::format("player_{}", static_cast<int>(Constants::ColorCyan));
   _player_material = new PlayerMaterial(_scene, material_texture_name.c_str(), "diffuse_level", "specular_level", "player-ao");

   TexturePool& pool = TexturePool::Instance();
   for (int i = 0; i < 10; i++)
   {
      const auto texture_name = std::format("player_{}", i + 1);
      _player_textures[i] = pool.getTexture(texture_name);
   }

   DataPaths::remove("data/winner");
}

void GameWinDrawable::updateWinAnimation(float time, float dt)
{
   _player_item->animate(time, dt);
}

void GameWinDrawable::startWinAnimation()
{
   _player_item->win();
}
