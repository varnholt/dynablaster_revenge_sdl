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
   void messageReceived(int sender_id, const std::string& message, bool typing_finished);

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
   void drawText(bool draw_user_input);
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

   PSD _psd;
   std::string _filename;
   std::vector<PSDLayer*> _psd_layers;
   PSDLayer* _line_edit_say_layer;
   PSDLayer* _player_name_layer;

   std::vector<AnimatedGameMessage*> _messages;
   BitmapFont* _font;

   std::string _message;
   Array<Vertex> _message_vertices;
   Array<Vertex> _nick_vertices;

   bool _active;
   FrameTimer _activation_time;

   int _cursor_position;

   // lazily-created 1x1 white texture + dynamic quad for drawCursor() - see
   // MenuPageTextEditItem::drawCursor()'s own doc comment for why this replaces the legacy
   // untextured glColor4ub'd quad (no fixed-function fallback in GLES3).
   unsigned int _cursor_texture;
   unsigned int _cursor_vertex_buffer;
};
