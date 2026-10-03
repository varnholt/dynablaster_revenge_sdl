// GLES3 port of client/src/game/gameplayernamedisplay.cpp.

#include "gameplayernamedisplay.h"

#include "gldevice.h"

#include "bombermanclient.h"
#include "menus/fontpool.h"
#include "playeritem.h"

#include "framework/globaltime.h"

#include "math/matrix.h"

#include "nodes/mesh.h"

#include "render/texturepool.h"

#include <cmath>
#define FONT_DISPLAY_DURATION 8000
#define FONT_DISPLAY_FADE_DURATION 3000
#define FONT_Y_OFFSET 30
#define ARROW_DISPLAY_DURATION 3000
#define ARROW_DISPLAY_FADE_DURATION 2000

namespace
{
struct ArrowVertex
{
   float x, y, z;
   float u, v;
};
}  // namespace

GamePlayerNameDisplay::GamePlayerNameDisplay()
{
}

bool GamePlayerNameDisplay::isActive() const
{
   return (_active_time.elapsed() < (FONT_DISPLAY_DURATION + FONT_DISPLAY_FADE_DURATION));
}

void GamePlayerNameDisplay::setPlayerData(std::map<int, PlayerItem*>& players)
{
   Matrix proj_mat = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix();

   const int width = 1920;
   const int height = 1080;

   _positions.clear();
   _names.clear();

   for (const auto& [playerId, player] : players)
   {
      if (!player->isKilled())
      {
         Matrix mat = player->getMesh()->getTransform() * proj_mat;
         mat = mat.xyw();

         Vector v = mat * Vector(0.0f, 0.0f, 0.0f);

         // 2d transform
         float t = 1.0f / v.z;

         v.x = (v.x * t + 1.0f) * 0.5f * width;
         v.y = (-v.y * t + 1.0f) * 0.5f * height;
         v.z = 0.0f;

         _positions.push_back(v);
         _names.push_back(player->getNick());

         if (player->getID() == BombermanClient::getInstance()->getPlayerId())
         {
            _arrow_position = player->getPosition();
         }
      }
   }
}

void GamePlayerNameDisplay::start()
{
   _active_time.restart();

   // show arrow only in large maps
   _show_arrow = false;
   GameInformation* info = BombermanClient::getInstance()->getCurrentGameInformation();

   switch (info->getMapDimensions())
   {
      case Constants::Dimension19x17:
      case Constants::Dimension25x21:
         _show_arrow = true;
         break;

      default:
         break;
   }
}

void GamePlayerNameDisplay::initialize()
{
   _font = &FontPool::Instance().get("outlined")->get();

   TexturePool& pool = TexturePool::Instance();
   _arrow_texture = pool.getTexture("data/game/arrow");

   _arrow_shader = activeDevice->loadShader("texalpha-vert.glsl", "texalpha-frag.glsl");
   _arrow_param_texture = activeDevice->getParameterIndex("tex");
   _arrow_param_alpha = activeDevice->getParameterIndex("alpha");

   // local quad in the arrow's own X/Z plane (Y fixed at 0) - positioned and animated per frame
   // via activeDevice->push(Matrix::position(...)) instead of rewriting vertex data every draw.
   const float width = 0.75f;
   const float height = 0.75f;

   _arrow_vertex_buffer = activeDevice->createVertexBuffer(4 * sizeof(ArrowVertex));
   ArrowVertex* vtx = (ArrowVertex*)activeDevice->lockVertexBuffer(_arrow_vertex_buffer);
   vtx[0] = {width, 0.0f, -height, 1.0f, 1.0f};
   vtx[1] = {width, 0.0f, height, 1.0f, 0.0f};
   vtx[2] = {-width, 0.0f, height, 0.0f, 0.0f};
   vtx[3] = {-width, 0.0f, -height, 0.0f, 1.0f};
   activeDevice->unlockVertexBuffer(_arrow_vertex_buffer);

   _arrow_index_buffer = activeDevice->createIndexBuffer(6 * sizeof(uint16_t));
   uint16_t* idx = (uint16_t*)activeDevice->lockIndexBuffer(_arrow_index_buffer);
   idx[0] = 0;
   idx[1] = 1;
   idx[2] = 2;
   idx[3] = 0;
   idx[4] = 2;
   idx[5] = 3;
   activeDevice->unlockIndexBuffer(_arrow_index_buffer);
}

void GamePlayerNameDisplay::draw() const
{
   initGlParameters();
   drawPlayTexts();

   if (_show_arrow)
      drawArrow();

   cleanupGlParameters();
}

void GamePlayerNameDisplay::initGlParameters() const
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
}

void GamePlayerNameDisplay::cleanupGlParameters() const
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

float GamePlayerNameDisplay::computeFontAlpha() const
{
   float alpha = 1.0f;

   if (_active_time.elapsed() > FONT_DISPLAY_DURATION)
   {
      alpha = 1.0f - ((_active_time.elapsed() - FONT_DISPLAY_DURATION) / (float)FONT_DISPLAY_FADE_DURATION);
   }

   return alpha;
}

float GamePlayerNameDisplay::computeArrowAlpha() const
{
   float alpha = 1.0f;

   if (_active_time.elapsed() > ARROW_DISPLAY_DURATION)
   {
      alpha = 1.0f - ((_active_time.elapsed() - ARROW_DISPLAY_DURATION) / (float)ARROW_DISPLAY_FADE_DURATION);
   }

   return alpha;
}

void GamePlayerNameDisplay::drawPlayTexts() const
{
   // ortho over the whole frame, restored for drawArrow() below - mirrors the original's
   // glMatrixMode(GL_PROJECTION)/glPushMatrix()/glLoadMatrixf(ortho)/.../glPopMatrix() bracket.
   GLDevice* device = static_cast<GLDevice*>(activeDevice);
   device->pushProjection();
   device->setProjectionMatrix(Matrix::ortho(0, 1920, 1080, 0, -1.0f, 1.0f));

   for (int i = 0; i < _positions.size(); i++)
   {
      const Vector& pos = _positions[i];
      const std::string& name = _names[i];

      _font->setColor(1.0f, 1.0f, 1.0f, computeFontAlpha());
      _font->buildVertices(0.1f, name.c_str(), pos.x, pos.y + FONT_Y_OFFSET, 0.0f);
      _font->draw();
   }

   device->popProjection();
}

void GamePlayerNameDisplay::drawArrow() const
{
   if (!BombermanClient::getInstance()->getCurrentPlayerInfo()->isKilled())
   {
      const float offset_z = 4.0f + std::sin(GlobalTime::Instance()->getTime() * 4.5f) * 0.5f;

      activeDevice->setShader(_arrow_shader);

      glBindTexture(GL_TEXTURE_2D, _arrow_texture.getTexture());
      activeDevice->bindSampler(_arrow_param_texture, 0);
      activeDevice->setParameter(_arrow_param_alpha, computeArrowAlpha() * 0.5f);

      activeDevice->push(Matrix::position(_arrow_position.x, _arrow_position.y, offset_z));

      glBindBuffer(GL_ARRAY_BUFFER, _arrow_vertex_buffer);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ArrowVertex), (GLvoid*)0);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ArrowVertex), (GLvoid*)(3 * sizeof(float)));

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _arrow_index_buffer);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);

      activeDevice->pop();
      activeDevice->setShader(0);
   }
}
