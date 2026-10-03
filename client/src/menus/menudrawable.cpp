#include "menudrawable.h"

#include "defaultshader.h"
#include "menu.h"
#include "menupagefadeanimation.h"
#include "menupageitem.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "math/matrix.h"

MenuDrawable::MenuDrawable(RenderDevice& device) : Drawable(device), _menu(std::make_unique<Menu>())
{
}

void MenuDrawable::initializeGL()
{
   _menu->initialize();

   _fade_in_animation = std::make_unique<MenuPageFadeAnimation>();
   _fade_in_animation->setFadeIn(true);
   _fade_in_animation->initialize();

   _fade_out_animation = std::make_unique<MenuPageFadeAnimation>();
   _fade_out_animation->setFadeIn(false);
   _fade_out_animation->initialize();

   _shader = getDefaultMenuShader();
   _alpha_parameter = getDefaultMenuShaderAlphaParam();
}

MenuDrawable::~MenuDrawable() = default;

void MenuDrawable::initGlParameters()
{
   if (const auto page = _menu->getCurrentPage())
   {
      const Matrix ortho = Matrix::ortho(0.0f, page->get().getWidth(), page->get().getHeight(), 0.0f, -1.0f, 1.0f);

      static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(ortho);
   }

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   // the blend func must be set explicitly: other drawables change it mid-frame and this is the
   // first draw call of a new frame
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MenuDrawable::drawMenuContents()
{
   initGlParameters();

   MenuPage& background = _menu->getBackground()->get();
   for (const auto& page : _menu->getPages())
   {
      if (!page->isActive())
      {
         continue;
      }

      const auto animation = page->getAnimation();

      if (animation)
      {
         animation->get().animate();
      }

      // re-established per page: during a cross-fade 2 pages are active at once
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // the blit pass below leaves an identity projection behind, so re-establish the page-space
      // ortho projection for every page, not just once before the loop
      const Matrix page_ortho = Matrix::ortho(0.0f, page->getWidth(), page->getHeight(), 0.0f, -1.0f, 1.0f);
      static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(page_ortho);

      if (!_frame_buffer)
      {
         _frame_buffer = std::make_unique<FrameBuffer>(page->getWidth(), page->getHeight());
      }
      else
      {
         _frame_buffer->setResolution(page->getWidth(), page->getHeight());
      }

      FrameBuffer::push();

      _frame_buffer->bind();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      activeDevice().setShader(_shader);

      background.render();
      page->render();

      _frame_buffer->unbind();
      FrameBuffer::pop();

      const float page_alpha = animation ? static_cast<const MenuPageFadeAnimation&>(animation->get()).getAlpha() : 1.0f;

      // the draws above may have changed blend state again
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // blit the composited page through the texalphaignore shader: the FBO's alpha is blend
      // residue and must be replaced, not multiplied. Shader and projection must be set before
      // push(), which uploads the combined MVP immediately.
      activeDevice().setShader(getFramebufferBlitShader());
      static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(Matrix());
      activeDevice().push(Matrix());
      activeDevice().setParameter(getFramebufferBlitShaderAlphaParam(), _alpha);

      _frame_buffer->draw(page_alpha);

      activeDevice().pop();
      activeDevice().setShader(0);
   }

   cleanupGlParameters();
}

void MenuDrawable::paintGL()
{
   drawMenuContents();
}

void MenuDrawable::setVisible(bool visible)
{
   if (isVisible() == visible)
   {
      return;
   }

   if (visible)
   {
      _reset_time = true;
      Drawable::setVisible(visible);

      setInputBlocked(true);
      startFadeInFrameBuffer();
   }
   else
   {
      setInputBlocked(true);
      startFadeOutFrameBuffer();
   }
}

void MenuDrawable::animateFadeFrameBuffer(float dt)
{
   const float value = dt * 0.01f;

   if (_fade_out)
   {
      _alpha -= value;

      if (_alpha <= 0.0f)
      {
         _alpha = 0.0f;
         _fade_out = false;

         Drawable::setVisible(false);

         setInputBlocked(false);
         visibleSignal(false);
      }
   }
   else if (_fade_in)
   {
      _alpha += value;

      if (_alpha >= 1.0f)
      {
         _alpha = 1.0f;
         _fade_in = false;

         setInputBlocked(false);
         visibleSignal(true);
      }
   }
}

void MenuDrawable::animate(float time)
{
   if (time == _time)
   {
      return;
   }

   float dt = 0.0f;

   // avoid large dt values on setVisble(true)
   if (_reset_time)
   {
      _reset_time = false;
   }
   else
   {
      dt = time - _time;
   }

   _time = time;

   for (const auto& page : _menu->getPages())
   {
      if (page->isActive())
      {
         for (const auto& item : page->getPageItems())
         {
            item->animate(time);
         }
      }
   }

   animateFadeFrameBuffer(dt);
}

void MenuDrawable::initializationFinished()
{
   // called externally after everything is set up
   if (const auto page = _menu->getCurrentPage())
   {
      pageChangedSignal(page->get().getFilename());
   }
}

void MenuDrawable::startFadeOutFrameBuffer()
{
   _alpha = 1.0f;
   _fade_out = true;
}

void MenuDrawable::startFadeInFrameBuffer()
{
   _alpha = 0.0f;
   _fade_in = true;
}

void MenuDrawable::fadeInFinished()
{
   Drawable::setVisible(true);
   visibleSignal(true);
}

void MenuDrawable::fadeOutFinished()
{
   Drawable::setVisible(false);
   visibleSignal(false);
}

void MenuDrawable::cleanupGlParameters()
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void MenuDrawable::setInputBlocked(bool blocked)
{
   _input_blocked = blocked;
}

bool MenuDrawable::isInputBlocked() const
{
   return _input_blocked;
}

void MenuDrawable::mousePressEvent(int x, int y)
{
   if (!isInputBlocked())
   {
      _menu->mousePressed(x, y);
   }
}

void MenuDrawable::mouseMoveEvent(int x, int y)
{
   _mouse_x = x;
   _mouse_y = y;

   _menu->mouseMoved(x, y);
}

void MenuDrawable::mouseReleaseEvent()
{
   if (!isInputBlocked())
   {
      _menu->mouseReleased();
   }
}

void MenuDrawable::keyPressEvent(const KeyEvent& event)
{
   _menu->keyPressed(event.key(), event.text());

   keyPressedSignal(event);
}

void MenuDrawable::pageChangeRequest(const std::string& name)
{
   setInputBlocked(true);

   MenuPage& previous = _menu->getCurrentPage()->get();
   MenuPage& current = _menu->getPageByName(name)->get();
   _menu->setCurrentPage(current);

   // set and connect animations
   previous.setAnimation(*_fade_out_animation);
   current.setAnimation(*_fade_in_animation);

   // cleanup previous connections
   _fade_in_animation->stoppedSignal.disconnectAll();
   _fade_out_animation->stoppedSignal.disconnectAll();

   // disable previous page when animation has finished
   _fade_out_animation->stoppedSignal.connect([&previous]() { previous.deactivate(); });

   _fade_out_animation->stoppedSignal.connect([&previous]() { previous.resetAnimation(); });

   _fade_in_animation->stoppedSignal.connect([&current]() { current.resetAnimation(); });

   _fade_in_animation->stoppedSignal.connect([this]() { pageChangeAnimationStopped(); });

   // activate the current page
   current.setActive(true);

   // start animations
   _fade_in_animation->start();
   _fade_out_animation->start();

   previous.unFocusAllItems();

   // signal page change
   pageChangedSignal(name);
   pageChangeActiveSignal(true);
}

Menu& MenuDrawable::getMenu()
{
   return *_menu;
}

void MenuDrawable::pageChangeAnimationStopped()
{
   // allow access to the page
   setInputBlocked(false);
   pageChangeActiveSignal(false);

   // re-trigger mouse move event
   mouseMoveEvent(_mouse_x, _mouse_y);

   pageChangeAnimationStoppedSignal();
}
