#pragma once

#include "framework/drawable.h"

#include <cstdint>
#include <string>

class BitmapFont;

//! stage, lives, score and the banners between the story mode's stages
class StoryHudDrawable : public Drawable
{
public:
   explicit StoryHudDrawable(RenderDevice& device);

   void initializeGL() override;
   void paintGL() override;
   void animate(float global_time) override;

   //! state as sent by the server (StoryStatePacket)
   void setState(int32_t stage, int32_t lives, int32_t score, int32_t state, int32_t enemies_left);

private:
   void showBanner(const std::string& title, const std::string& subtitle);

   BitmapFont* _font = nullptr;
   int32_t _stage = 0;
   int32_t _lives = 0;
   int32_t _score = 0;
   int32_t _state = -1;
   int32_t _enemies_left = 0;
   std::string _banner_title;
   std::string _banner_subtitle;
   float _banner_time = -1.0f;
   float _last_time = 0.0f;
};
