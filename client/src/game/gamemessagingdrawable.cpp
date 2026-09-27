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

#include <cmath>
#include <algorithm>

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

GameMessagingDrawable::GameMessagingDrawable(RenderDevice* dev)
    : Drawable(dev),
      mLineEditSayLayer(nullptr),
      mPlayerNameLayer(nullptr),
      mFont(nullptr),
      mActive(false),
      mCursorPosition(0),
      mCursorTexture(0),
      mCursorVertexBuffer(0)
{
   mFilename = "data/game/messaging_bar.psd";

   BombermanClient::getInstance()->gameStartedSignal.connect([this]() { disableIngameMessaging(); });
   GameStateMachine::getInstance()->stateChangedSignal.connect([this]() { gameStateChanged(); });
}

GameMessagingDrawable::~GameMessagingDrawable()
{
   for (auto* layer : mPsdLayers)
   {
      delete layer;
   }
   mPsdLayers.clear();
}

void GameMessagingDrawable::messageReceived(int /*senderId*/, const std::string& text, bool typingFinished)
{
   if (mVisible)
   {
      if (StringUtils::trim(text).empty())
         return;

      if (typingFinished)
      {
         const std::vector<std::string> lines = WordWrap::wrap(text, CLIENT_MESSAGE_TEXT_MAXIMUM);

         // prepend nick to other lines if available
         std::vector<std::string> processedLines;
         std::string message = lines.at(0);
         processedLines.push_back(message);

         if (lines.size() > 1)
         {
            size_t startIndex = message.find(":");
            if (startIndex != std::string::npos)
            {
               std::string nick = message.substr(0, startIndex);

               if (StringUtils::trim(message) == nick + ":")
                  processedLines.clear();

               for (size_t i = 1; i < lines.size(); i++)
               {
                  processedLines.push_back(nick + ": " + lines.at(i));
               }
            }
         }

         for (const std::string& line : processedLines)
         {
            mFont->setColor(1.0f, 1.0f, 1.0f, 1.0f);
            mFont->buildVertices(FONT_SCALE_MESSAGE, line.c_str(), MESSAGE_OFFSET_X, MESSAGE_OFFSET_Y - MESSAGE_STACK_OFFSET);

            AnimatedGameMessage* message = new AnimatedGameMessage();
            message->setMessage(line);
            message->setVertices(mFont->getVertices());
            message->initialize();

            mMessages.insert(mMessages.begin(), message);

            message->expiredSignal.connect([this, message]() { popMessage(message); });
         }
      }
   }
}

void GameMessagingDrawable::clearMessage()
{
   mMessage.clear();
}

void GameMessagingDrawable::keyPressEvent(const KeyEvent& event)
{
   if (GameStateMachine::getInstance()->getState() == Constants::GameActive)
   {
      if (event.key() == SDLK_RETURN || event.key() == SDLK_KP_ENTER)
      {
         bool wasActive = isActive();
         toggleActive();

         if (wasActive)
         {
            if (!mMessage.empty())
            {
               BombermanClient::getInstance()->sendMessage(mMessage, true);
               clearMessage();
            }
         }
         else
         {
            setCursorPosition(static_cast<int>(mMessage.length()));
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
                  if (!mMessage.empty())
                     mMessage.pop_back();
                  moveCursorLeft();
               }
               else
               {
                  if (getCursorPosition() > 0)
                  {
                     mMessage.erase(getCursorPosition() - 1, 1);
                     moveCursorLeft();
                  }
               }
            }
            else if (event.key() == SDLK_DELETE)
            {
               if (!isCursorAtEnd())
               {
                  mMessage.erase(getCursorPosition(), 1);
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
                  if (static_cast<int>(mMessage.length()) < MESSAGE_LENGTH_MAX)
                     mMessage.append(text);
               }
               else
               {
                  mMessage.replace(getCursorPosition(), 1, text);
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
   int i0 = std::max(mCursorPosition - MESSAGE_FIELD_WIDTH, 0);
   const std::string visibleText = mMessage.substr(std::min(static_cast<size_t>(i0), mMessage.size()), MESSAGE_FIELD_WIDTH);

   mFont->setColor(1.0f, 1.0f, 1.0f, 1.0f);
   mFont->buildVertices(FONT_SCALE_MESSAGE, visibleText.c_str(), mLineEditSayLayer->getLeft(), LINEEDIT_SAY_Y);

   mMessageVertices.copy(mFont->getVertices());
}

void GameMessagingDrawable::buildNickVertices()
{
   PlayerInfo* playerInfo = BombermanClient::getInstance()->getCurrentPlayerInfo();
   if (playerInfo)
   {
      mFont->setColor(1.0f, 1.0f, 1.0f, 1.0f);
      mFont->buildVertices(FONT_SCALE_NICK, playerInfo->getNick().c_str(), 0, 0);
      mNickVertices.copy(mFont->getVertices());
   }
   else
   {
      mNickVertices.clear();
   }
}

void GameMessagingDrawable::popMessage(AnimatedGameMessage* message)
{
   auto it = std::find(mMessages.begin(), mMessages.end(), message);
   if (it != mMessages.end())
   {
      mMessages.erase(it);
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
      mMessage = BombermanClient::getInstance()->getMessage();
      updateMessageVertices();
   }
}

void GameMessagingDrawable::initializeGL()
{
   mFont = FontPool::Instance()->get("default");

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
   mActive = active;
}

bool GameMessagingDrawable::isActive() const
{
   return mActive;
}

void GameMessagingDrawable::toggleActive()
{
   setActive(!isActive());
   restartActivationTime();
}

void GameMessagingDrawable::drawText(bool drawUserInput)
{
   // the legacy 5-pass glTranslatef offset trick (4 black outline passes at +-1px, then a real
   // pass) becomes 5 real push()/pop() brackets with a translated world matrix - GLES3 has no
   // matrix stack to abuse for this, but the effect is identical.
   const float offsets[5][3] = {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.75f, 0.0f, 0.0f}};

   for (int f = 0; f < 5; f++)
   {
      const float col = offsets[f][0];
      const float x = offsets[f][1];
      const float y = offsets[f][2];

      float ty = 0.0f;

      if (isActive() && drawUserInput)
      {
         mFont->setColor(col, col, col, 1.0f);
         activeDevice->push(Matrix::position(x, y + ty, 0.0f));
         mFont->draw(mMessageVertices);
         activeDevice->pop();
      }

      for (int i = 0; i < mMessages.size(); i++)
      {
         AnimatedGameMessage* message = mMessages.at(i);

         ty -= MESSAGE_OFFSET_DIFF;

         mFont->setColor(col, col, col, 1.0f - (i * 0.08f));
         activeDevice->push(Matrix::position(x, y + ty, 0.0f));
         mFont->draw(message->getVertices());
         activeDevice->pop();
      }
   }
}

void GameMessagingDrawable::paintGL()
{
   initGlParameters();

   bool drawUserInput = drawMessageOverlay();

   drawText(drawUserInput);

   if (drawUserInput)
      drawCursor();

   cleanupGlParameters();
}

bool GameMessagingDrawable::drawMessageOverlay()
{
   bool fullyVisible = false;

   int time = getActivationTime().isValid() ? static_cast<int>(getActivationTime().elapsed()) : ACTIVATION_TIME;
   float factor = (std::max(ACTIVATION_TIME - time, 0)) * ACTIVATION_FACTOR;

   if (!isActive())
      factor = 1.0f - factor;

   int offset = MESSAGE_LAYER_POSITION + static_cast<int>(ACTIVATION_OFFSET * factor);

   fullyVisible = (offset == MESSAGE_LAYER_POSITION);

   for (int layerIndex = 0; layerIndex < mPsdLayers.size(); layerIndex++)
   {
      PSDLayer* layer = mPsdLayers[layerIndex];
      layer->render(0, static_cast<float>(offset), 1.0f - factor);
   }

   if (!getMessage().empty())
   {
      mFont->setColor(1.0f, 1.0f, 1.0f, 1.0f);
      activeDevice->push(Matrix::position(static_cast<float>(mPlayerNameLayer->getLeft()), offset + LABEL_PLAYER_OFFSET_Y, 0.0f));
      mFont->draw(mMessageVertices);
      activeDevice->pop();
   }

   return fullyVisible;
}

void GameMessagingDrawable::restartActivationTime()
{
   mActivationTime.restart();
}

const FrameTimer& GameMessagingDrawable::getActivationTime() const
{
   return mActivationTime;
}

const std::string& GameMessagingDrawable::getMessage() const
{
   return mMessage;
}

void GameMessagingDrawable::setMessage(const std::string& value)
{
   mMessage = value;
}

void GameMessagingDrawable::initializeLayers()
{
   mPsd.load(mFilename.c_str());

   for (int l = 0; l < mPsd.getLayerCount(); l++)
   {
      PSD::Layer* layer = mPsd.getLayer(l);
      PSDLayer* renderLayer = new PSDLayer(layer);

      mPsdLayers.push_back(renderLayer);

      if (layer->getName() == std::string(LINEEDIT_SAY))
      {
         mLineEditSayLayer = renderLayer;
      }
      else if (layer->getName() == std::string(LABEL_PLAYER_NAME))
      {
         mPlayerNameLayer = renderLayer;
      }
   }
}

void GameMessagingDrawable::setCursorPosition(int index)
{
   mCursorPosition = index;

   updateMessageVertices();
}

int GameMessagingDrawable::getCursorPosition() const
{
   return mCursorPosition;
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

   mFont->getCursor(FONT_SCALE_MESSAGE, getCursorPosition(), left, right, top, bottom);

   if (mCursorTexture == 0)
   {
      unsigned int white = 0xFFFFFFFF;
      mCursorTexture = activeDevice->createTexture(&white, 1, 1, 0);
   }

   glBindTexture(GL_TEXTURE_2D, mCursorTexture);

   glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);

   const float quad[] = {
      left, top, -1.0f, 0.0f, 0.0f, right, top,    -1.0f, 1.0f, 0.0f, right, bottom, -1.0f, 1.0f, 1.0f,
      left, top, -1.0f, 0.0f, 0.0f, right, bottom, -1.0f, 1.0f, 1.0f, left,  bottom, -1.0f, 0.0f, 1.0f,
   };

   if (mCursorVertexBuffer == 0)
      mCursorVertexBuffer = activeDevice->createVertexBuffer(sizeof(quad), true);
   else
      activeDevice->allocateVertexBuffer(mCursorVertexBuffer, sizeof(quad), true);

   void* dst = activeDevice->lockVertexBuffer(mCursorVertexBuffer, sizeof(quad));
   std::memcpy(dst, quad, sizeof(quad));
   activeDevice->unlockVertexBuffer(mCursorVertexBuffer);

   activeDevice->push(Matrix());
   activeDevice->setParameter(activeDevice->getParameterIndex("alpha"), (128.0f / 255.0f) * alpha);

   glBindBuffer(GL_ARRAY_BUFFER, mCursorVertexBuffer);
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
