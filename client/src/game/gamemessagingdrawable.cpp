// GLES3 port of client/src/game/gamemessagingdrawable.cpp.

#include "gamemessagingdrawable.h"
#include <cstring>

#include <SDL3/SDL_keycode.h>

#include "bombermanclient.h"
#include "gamestatemachine.h"
#include "menus/fontpool.h"
#include "menus/psdlayer.h"
#include "stringutils.h"
#include "wordwrap.h"

#include "constants.h"
#include "playerinfo.h"

#include "framework/globaltime.h"

#include "gldevice.h"
#include "timer.h"

#include <algorithm>
#include <cmath>

#define LINEEDIT_SAY "lineedit_say"
#define LINEEDIT_SAY_Y 1057
#define LABEL_PLAYER_NAME "chat_player_name"
#define LABEL_PLAYER_OFFSET_Y 30
#define MESSAGE_LAYER_POSITION 1020
#define MESSAGE_STACK_OFFSET 35
#define MESSAGE_OFFSET_X 5
#define MESSAGE_OFFSET_Y 1060
#define MESSAGE_OFFSET_DIFF 25.0f
#define MESSAGE_LENGTH_MAX 400
#define FONT_SCALE_NICK 0.08f
#define FONT_SCALE_MESSAGE 0.15f
#define ACTIVATION_TIME 300
#define ACTIVATION_FACTOR 0.003333333f
#define ACTIVATION_OFFSET 100
#define CURSOR_UPDATE_TIME 0.5f
#define MESSAGE_FIELD_WIDTH 80

GameMessagingDrawable::GameMessagingDrawable(RenderDevice* dev) : Drawable(dev)
{
   _filename = "data/game/messaging_bar.psd";

   BombermanClient::getInstance()->gameStartedSignal.connect([this]() { disableIngameMessaging(); });
   GameStateMachine::getInstance()->stateChangedSignal.connect([this]() { gameStateChanged(); });
}

GameMessagingDrawable::~GameMessagingDrawable() = default;

void GameMessagingDrawable::messageReceived(int /*sender_id*/, const std::string& text, bool typing_finished)
{
   if (_visible)
   {
      if (StringUtils::trim(text).empty())
         return;

      if (typing_finished)
      {
         const std::vector<std::string> lines = WordWrap::wrap(text, CLIENT_MESSAGE_TEXT_MAXIMUM);

         // prepend nick to other lines if available
         std::vector<std::string> processed_lines;
         std::string message = lines.at(0);
         processed_lines.push_back(message);

         if (lines.size() > 1)
         {
            size_t start_index = message.find(":");
            if (start_index != std::string::npos)
            {
               std::string nick = message.substr(0, start_index);

               if (StringUtils::trim(message) == nick + ":")
                  processed_lines.clear();

               for (size_t i = 1; i < lines.size(); i++)
               {
                  processed_lines.push_back(nick + ": " + lines.at(i));
               }
            }
         }

         for (const std::string& line : processed_lines)
         {
            _font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
            _font->buildVertices(FONT_SCALE_MESSAGE, line.c_str(), MESSAGE_OFFSET_X, MESSAGE_OFFSET_Y - MESSAGE_STACK_OFFSET);

            AnimatedGameMessage* message = new AnimatedGameMessage();
            message->setMessage(line);
            message->setVertices(_font->getVertices());
            message->initialize();

            _messages.insert(_messages.begin(), message);

            message->expiredSignal.connect([this, message]() { popMessage(message); });
         }
      }
   }
}

void GameMessagingDrawable::clearMessage()
{
   _message.clear();
}

void GameMessagingDrawable::keyPressEvent(const KeyEvent& event)
{
   if (GameStateMachine::getInstance()->getState() == Constants::GameActive)
   {
      if (event.key() == SDLK_RETURN || event.key() == SDLK_KP_ENTER)
      {
         bool was_active = isActive();
         toggleActive();

         if (was_active)
         {
            if (!_message.empty())
            {
               BombermanClient::getInstance()->sendMessage(_message, true);
               clearMessage();
            }
         }
         else
         {
            setCursorPosition(static_cast<int>(_message.length()));
         }
      }
      else
      {
         if (isActive())
         {
            if (event.key() == SDLK_ESCAPE)
            {
               clearMessage();
               toggleActive();
            }
            else if (event.key() == SDLK_BACKSPACE)
            {
               if (isCursorAtEnd())
               {
                  if (!_message.empty())
                     _message.pop_back();
                  moveCursorLeft();
               }
               else
               {
                  if (getCursorPosition() > 0)
                  {
                     _message.erase(getCursorPosition() - 1, 1);
                     moveCursorLeft();
                  }
               }
            }
            else if (event.key() == SDLK_DELETE)
            {
               if (!isCursorAtEnd())
               {
                  _message.erase(getCursorPosition(), 1);
               }
            }
            else if (event.key() == SDLK_LEFT)
            {
               moveCursorLeft();
            }
            else if (event.key() == SDLK_RIGHT)
            {
               moveCursorRight();
            }
            else if (event.key() == SDLK_HOME)
            {
               moveCursorToStart();
            }
            else if (event.key() == SDLK_END)
            {
               moveCursorToEnd();
            }
            else if (!event.text().empty())
            {
               const std::string& text = event.text();

               if (isCursorAtEnd())
               {
                  if (static_cast<int>(_message.length()) < MESSAGE_LENGTH_MAX)
                     _message.append(text);
               }
               else
               {
                  _message.replace(getCursorPosition(), 1, text);
               }

               moveCursorRight();
            }
         }
      }

      updateMessageVertices();
   }
}

void GameMessagingDrawable::setVisible(bool visible)
{
   Drawable::setVisible(visible);

   if (visible)
   {
      // build nick vertices only once
      buildNickVertices();
   }
}

void GameMessagingDrawable::updateMessageVertices()
{
   int i0 = std::max(_cursor_position - MESSAGE_FIELD_WIDTH, 0);
   const std::string visible_text = _message.substr(std::min(static_cast<size_t>(i0), _message.size()), MESSAGE_FIELD_WIDTH);

   _font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
   _font->buildVertices(FONT_SCALE_MESSAGE, visible_text.c_str(), _line_edit_say_layer->getLeft(), LINEEDIT_SAY_Y);

   _message_vertices.copy(_font->getVertices());
}

void GameMessagingDrawable::buildNickVertices()
{
   PlayerInfo* player_info = BombermanClient::getInstance()->getCurrentPlayerInfo();
   if (player_info)
   {
      _font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
      _font->buildVertices(FONT_SCALE_NICK, player_info->getNick().c_str(), 0, 0);
      _nick_vertices.copy(_font->getVertices());
   }
   else
   {
      _nick_vertices.clear();
   }
}

void GameMessagingDrawable::popMessage(AnimatedGameMessage* message)
{
   auto it = std::find(_messages.begin(), _messages.end(), message);
   if (it != _messages.end())
   {
      _messages.erase(it);
   }

   Timer::singleShot(0, [message]() { delete message; });
}

void GameMessagingDrawable::disableIngameMessaging()
{
   setActive(false);
}

void GameMessagingDrawable::gameStateChanged()
{
   if (GameStateMachine::getInstance()->getState() == Constants::GameStopped)
   {
      disableIngameMessaging();
   }
   else if (GameStateMachine::getInstance()->getState() == Constants::GameActive)
   {
      // copy buffer from client to textedit
      _message = BombermanClient::getInstance()->getMessage();
      updateMessageVertices();
   }
}

void GameMessagingDrawable::initializeGL()
{
   _font = FontPool::Instance()->get("default");

   initializeLayers();
}

void GameMessagingDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, 1920, 1080, 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice*>(activeDevice)->setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice->setShader(0);
}

void GameMessagingDrawable::cleanupGlParameters()
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void GameMessagingDrawable::setActive(bool active)
{
   _active = active;
}

bool GameMessagingDrawable::isActive() const
{
   return _active;
}

void GameMessagingDrawable::toggleActive()
{
   setActive(!isActive());
   restartActivationTime();
}

void GameMessagingDrawable::drawText(bool draw_user_input)
{
   // Pass each outline and message offset to the font shader explicitly.
   const float offsets[5][3] = {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.75f, 0.0f, 0.0f}};

   for (int f = 0; f < 5; f++)
   {
      const float col = offsets[f][0];
      const float x = offsets[f][1];
      const float y = offsets[f][2];

      float ty = 0.0f;

      if (isActive() && draw_user_input)
      {
         _font->setColor(col, col, col, 1.0f);
         _font->draw(_message_vertices, Matrix::position(x, y + ty, 0.0f));
      }

      for (int i = 0; i < _messages.size(); i++)
      {
         AnimatedGameMessage* message = _messages.at(i);

         ty -= MESSAGE_OFFSET_DIFF;

         _font->setColor(col, col, col, 1.0f - (i * 0.08f));
         _font->draw(message->getVertices(), Matrix::position(x, y + ty, 0.0f));
      }
   }
}

void GameMessagingDrawable::paintGL()
{
   initGlParameters();

   bool draw_user_input = drawMessageOverlay();

   drawText(draw_user_input);

   if (draw_user_input)
      drawCursor();

   cleanupGlParameters();
}

bool GameMessagingDrawable::drawMessageOverlay()
{
   bool fully_visible = false;

   int time = getActivationTime().isValid() ? static_cast<int>(getActivationTime().elapsed()) : ACTIVATION_TIME;
   float factor = (std::max(ACTIVATION_TIME - time, 0)) * ACTIVATION_FACTOR;

   if (!isActive())
      factor = 1.0f - factor;

   int offset = MESSAGE_LAYER_POSITION + static_cast<int>(ACTIVATION_OFFSET * factor);

   fully_visible = (offset == MESSAGE_LAYER_POSITION);

   for (const auto& layer : _psd_layers)
   {
      layer->render(0, static_cast<float>(offset), 1.0f - factor);
   }

   if (!getMessage().empty())
   {
      _font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
      _font->draw(_message_vertices,
                  Matrix::position(static_cast<float>(_player_name_layer->getLeft()), offset + LABEL_PLAYER_OFFSET_Y, 0.0f));
   }

   return fully_visible;
}

void GameMessagingDrawable::restartActivationTime()
{
   _activation_time.restart();
}

const FrameTimer& GameMessagingDrawable::getActivationTime() const
{
   return _activation_time;
}

const std::string& GameMessagingDrawable::getMessage() const
{
   return _message;
}

void GameMessagingDrawable::setMessage(const std::string& value)
{
   _message = value;
}

void GameMessagingDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   for (int l = 0; l < _psd.getLayerCount(); l++)
   {
      PSD::Layer* layer = _psd.getLayer(l);
      auto owned_layer = std::make_unique<PSDLayer>(layer);
      PSDLayer* render_layer = owned_layer.get();

      _psd_layers.push_back(std::move(owned_layer));

      if (layer->getName() == std::string(LINEEDIT_SAY))
      {
         _line_edit_say_layer = render_layer;
      }
      else if (layer->getName() == std::string(LABEL_PLAYER_NAME))
      {
         _player_name_layer = render_layer;
      }
   }
}

void GameMessagingDrawable::setCursorPosition(int index)
{
   _cursor_position = index;

   updateMessageVertices();
}

int GameMessagingDrawable::getCursorPosition() const
{
   return _cursor_position;
}

void GameMessagingDrawable::moveCursorRight()
{
   setCursorPosition(std::min(getCursorPosition() + 1, static_cast<int>(getMessage().length())));
}

void GameMessagingDrawable::moveCursorLeft()
{
   setCursorPosition(std::max(getCursorPosition() - 1, 0));
}

void GameMessagingDrawable::moveCursorToStart()
{
   setCursorPosition(0);
}

void GameMessagingDrawable::moveCursorToEnd()
{
   setCursorPosition(static_cast<int>(getMessage().length()));
}

bool GameMessagingDrawable::isCursorAtEnd() const
{
   return (getCursorPosition() == static_cast<int>(getMessage().length()));
}

void GameMessagingDrawable::drawCursor()
{
   // see MenuPageTextEditItem::drawCursor()'s own doc comment - same lazily-created 1x1 white
   // texture + dynamic quad replacement for the legacy untextured glColor4ub'd quad.
   float alpha = 0.25f + std::fmod(GlobalTime::Instance()->getTime(), 0.5f);

   float left = 0.0f;
   float right = 0.0f;
   float top = 0.0f;
   float bottom = 0.0f;

   _font->getCursor(FONT_SCALE_MESSAGE, getCursorPosition(), left, right, top, bottom);

   if (_cursor_texture == 0)
   {
      uint32_t white = 0xFFFFFFFF;
      _cursor_texture = activeDevice->createTexture(&white, 1, 1, 0);
   }

   glBindTexture(GL_TEXTURE_2D, _cursor_texture);

   glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);

   const float quad[] = {
      left, top, -1.0f, 0.0f, 0.0f, right, top,    -1.0f, 1.0f, 0.0f, right, bottom, -1.0f, 1.0f, 1.0f,
      left, top, -1.0f, 0.0f, 0.0f, right, bottom, -1.0f, 1.0f, 1.0f, left,  bottom, -1.0f, 0.0f, 1.0f,
   };

   if (_cursor_vertex_buffer == 0)
      _cursor_vertex_buffer = activeDevice->createVertexBuffer(sizeof(quad), true);
   else
      activeDevice->allocateVertexBuffer(_cursor_vertex_buffer, sizeof(quad), true);

   void* dst = activeDevice->lockVertexBuffer(_cursor_vertex_buffer, sizeof(quad));
   std::memcpy(dst, quad, sizeof(quad));
   activeDevice->unlockVertexBuffer(_cursor_vertex_buffer);

   activeDevice->push(Matrix());
   activeDevice->setParameter(activeDevice->getParameterIndex("alpha"), (128.0f / 255.0f) * alpha);

   glBindBuffer(GL_ARRAY_BUFFER, _cursor_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)0);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice->pop();

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
