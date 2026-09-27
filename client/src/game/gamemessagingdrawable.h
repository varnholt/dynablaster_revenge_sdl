#pragma once

// GLES3 port of client/src/game/gamemessagingdrawable.cpp.

#include "animatedgamemessage.h"
#include "drawable.h"

#include "framework/frametimer.h"
#include "image/psd.h"

#include <string>

#include <vector>

class BitmapFont;
class PSDLayer;

class GameMessagingDrawable : public Drawable
{
public:
   GameMessagingDrawable(RenderDevice* dev);
   virtual ~GameMessagingDrawable();

   void updateMessageVertices();

   void initializeGL();
   void paintGL();

   virtual void keyPressEvent(const KeyEvent& event);
   virtual void setVisible(bool visible);

   const std::string& getMessage() const;
   void setMessage(const std::string& value);

public:
   void messageReceived(int senderId, const std::string& message, bool typingFinished);

protected:
   void popMessage(AnimatedGameMessage* message);
   void disableIngameMessaging();
   void gameStateChanged();

protected:
   void initGlParameters();
   void cleanupGlParameters();

   void setActive(bool active);
   bool isActive() const;
   void toggleActive();

   void buildNickVertices();
   void drawText(bool drawUserInput);
   void clearMessage();

   bool isCursorAtEnd() const;
   void setCursorPosition(int);
   int getCursorPosition() const;
   void moveCursorRight();
   void moveCursorLeft();
   void moveCursorToStart();
   void moveCursorToEnd();
   void drawCursor();
   void restartCursorTime();

   void initializeLayers();
   bool drawMessageOverlay();

   void restartActivationTime();
   const FrameTimer& getActivationTime() const;

   PSD mPsd;
   std::string mFilename;
   std::vector<PSDLayer*> mPsdLayers;
   PSDLayer* mLineEditSayLayer;
   PSDLayer* mPlayerNameLayer;

   std::vector<AnimatedGameMessage*> mMessages;
   BitmapFont* mFont;

   std::string mMessage;
   Array<Vertex> mMessageVertices;
   Array<Vertex> mNickVertices;

   bool mActive;
   FrameTimer mActivationTime;

   int mCursorPosition;

   // lazily-created 1x1 white texture + dynamic quad for drawCursor() - see
   // MenuPageTextEditItem::drawCursor()'s own doc comment for why this replaces the legacy
   // untextured glColor4ub'd quad (no fixed-function fallback in GLES3).
   unsigned int mCursorTexture;
   unsigned int mCursorVertexBuffer;
};
