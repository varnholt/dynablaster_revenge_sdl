#pragma once

// GLES3 port of client/src/game/gamemessagingdrawable.cpp.

#include "animatedgamemessage.h"
#include "drawable.h"

#include "framework/frametimer.h"
#include "image/psd.h"

#include <string>

#include <vector>
#include <cstdint>
#include <memory>

class BitmapFont;
class PSDLayer;

class GameMessagingDrawable : public Drawable
{
public:
   GameMessagingDrawable(RenderDevice* dev);
   ~GameMessagingDrawable() override;

   void updateMessageVertices();

   void initializeGL() override;
   void paintGL() override;

   void keyPressEvent(const KeyEvent& event) override;
   void setVisible(bool visible) override;

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
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;
   PSDLayer* _line_edit_say_layer = nullptr;
   PSDLayer* _player_name_layer = nullptr;

   std::vector<AnimatedGameMessage*> _messages;
   BitmapFont* _font = nullptr;

   std::string _message;
   Array<Vertex> _message_vertices;
   Array<Vertex> _nick_vertices;

   bool _active = false;
   FrameTimer _activation_time;

   int _cursor_position = 0;

   // lazily-created 1x1 white texture + dynamic quad for drawCursor() - see
   // MenuPageTextEditItem::drawCursor()'s own doc comment for why this replaces the legacy
   // untextured glColor4ub'd quad (no fixed-function fallback in GLES3).
   uint32_t _cursor_texture = 0;
   uint32_t _cursor_vertex_buffer = 0;
};
