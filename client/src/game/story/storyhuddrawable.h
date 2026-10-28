#pragma once

#include "framework/drawable.h"
#include "image/psd.h"

#include <cstdint>
#include <memory>
#include <string>

class BitmapFont;
class PSDLayer;

//! stage, lives, score and the banners between the story mode's stages
class StoryHudDrawable : public Drawable
{
public:
   explicit StoryHudDrawable(RenderDevice& device);
   ~StoryHudDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float global_time) override;

   //! state as sent by the server (StoryStatePacket)
   void setState(int32_t stage, int32_t lives, int32_t score, int32_t state, int32_t enemies_left);

   //! shows the pause screen
   void setPaused(bool paused);

   //! after the last life: asks whether to continue, counting down, yes or no selected
   void setContinuePrompt(bool shown, bool yes);

private:
   void showBanner(const std::string& title, const std::string& subtitle);

   //! stage, lives, score and enemies in a row right of the time panel
   void drawStats(BitmapFont& font);

   //! the big "PAUSED" in the countdown's look, with the hint how to continue
   void drawPaused(BitmapFont& font);

   //! "GAME OVER" and the password kept up, below them "CONTINUE?", yes and no and the seconds left
   void drawContinuePrompt(BitmapFont& font);

   BitmapFont* _font = nullptr;
   PSD _paused_psd;
   std::unique_ptr<PSDLayer> _paused_layer;
   float _paused_time = 0.0f;
   int32_t _stage = 0;
   int32_t _lives = 0;
   int32_t _score = 0;
   int32_t _state = -1;
   int32_t _enemies_left = 0;
   std::string _banner_title;
   std::string _banner_subtitle;
   float _banner_time = -1.0f;
   float _last_time = 0.0f;
   bool _paused = false;
   bool _continue_prompt = false;
   bool _continue_yes = true;
   float _continue_time = 0.0f;
};
