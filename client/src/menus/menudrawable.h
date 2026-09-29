#pragma once

#include "framework/drawable.h"

#include "signal.h"

#include "image/psd.h"

#include <cstdint>
#include <memory>

class Menu;
class MenuPageFadeAnimation;
class FrameBuffer;

/// \brief draws the menu pages; each active page is composited into an own FrameBuffer (sized to
/// the page, created lazily) and blitted to the screen for the page cross-fade.
class MenuDrawable : public Drawable
{
public:
   MenuDrawable(RenderDevice*);
   ~MenuDrawable() override;

   Menu* getMenu();

   void initializeGL() override;

   void paintGL() override;

   void setVisible(bool visible) override;

   void mousePressEvent(int x, int y) override;

   void mouseMoveEvent(int x, int y) override;

   void mouseReleaseEvent() override;

   void keyPressEvent(const KeyEvent& event) override;

   void animate(float global_time) override;

   //! initialization finished
   void initializationFinished();

   //! page was changed
   Signal<const std::string&> pageChangedSignal;

   //! page change active
   Signal<bool> pageChangeActiveSignal;

   //! visible or not
   Signal<bool> visibleSignal;

   //! page change finished
   Signal<> pageChangeAnimationStoppedSignal;

   //! signal key pressed event
   Signal<const KeyEvent&> keyPressedSignal;

   //! setter for active page by name
   void pageChangeRequest(const std::string&);

protected:
   //! page change has been finished
   void pageChangeAnimationStopped();

   //! fade in has finished
   void fadeInFinished();

   //! fade out has finished
   void fadeOutFinished();

   void initGlParameters();

   void cleanupGlParameters();

   void setInputBlocked(bool);

   bool isInputBlocked() const;

   void drawMenuContents();

   //! animate fade out of frame buffer
   void animateFadeFrameBuffer(float dt);

   //! fade out menu framebuffer
   void startFadeOutFrameBuffer();

   //! fade in menu framebuffer
   void startFadeInFrameBuffer();

   // declared before the animations: the pages only observe them, the animations' stopped
   // callbacks capture page pointers
   std::unique_ptr<Menu> _menu;

   std::unique_ptr<MenuPageFadeAnimation> _fade_in_animation;
   std::unique_ptr<MenuPageFadeAnimation> _fade_out_animation;

   bool _input_blocked = false;

   int _mouse_x = 0;
   int _mouse_y = 0;

   float _time = 0.0f;

   //! fade out framebuffer flag
   bool _fade_out = false;

   //! fade in framebuffer flag
   bool _fade_in = false;

   float _alpha = 0.0f;

   //! reset time on setVisible(true)
   bool _reset_time = false;

   //! alpha shader
   uint32_t _shader = 0;
   int _alpha_parameter = -1;

   //! page cross-fade render target
   std::unique_ptr<FrameBuffer> _frame_buffer;
};
