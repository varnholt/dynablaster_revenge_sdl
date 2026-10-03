#pragma once

#include "menupagetextedit.h"

#include "framework/frametimer.h"
#include "vertex.h"

#include <memory>
#include <vector>

class MenuPageListItemElement : public MenuPageTextEditItem
{
public:
   MenuPageListItemElement();
   ~MenuPageListItemElement() override;

   void initialize() override;

   using MenuPageTextEditItem::draw;
   void draw(float x, float y, float opacity = 1.0f);

   void setIndex(int index);

   void setHeight(int height);

   void setWidth(int width);

   int getHeight() const;

   int getWidth() const;

   void setX(float x);

   void setY(float y);

   float getX() const;

   float getY() const;

   std::vector<Vertex> getBoundingRectVertices(float x, float y);

   bool isFadingOut();

   void stopFadeOut();

   float getFadeOutValue();

   void setOverrideAlpha(bool);

   bool isOverrideAlphaActive() const;

   const FrameTimer& getFocusOutTime() const;

   void setFocus(bool) override;

   void setActive(bool) override;

private:
   // placeholder layers owned by the element; the base class only observes them
   std::unique_ptr<PSDLayer> _dummy_layer_active;
   std::unique_ptr<PSDLayer> _dummy_layer_inactive;

   int _index = 0;
   int _width = 0;
   int _height = 0;

   float _x = 0.0f;
   float _y = 0.0f;

   FrameTimer _focus_out_time;
   bool _fade_out = false;

   bool _override_alpha = false;
};
