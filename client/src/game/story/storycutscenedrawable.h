#pragma once

#include "framework/drawable.h"
#include "math/matrix.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class BitmapFont;
class GlowSprites;
class GltfModel;
class NightSky;

//! the story mode's cutscenes after the original's: the intro (the dragon breaks out of Dr's lab with the princess
//! and the black bomberman, the doctor and the player panic, the dragon flies off to its castle on the mountain)
//! and the ending (in the castle's machine room the player catches up with the black bomberman, the princess runs to
//! the player, the black bomberman sits down and cries).
//!
//! Staged in Blender (assets/intro/build_intro.py, build_ending.py) and played from a timeline in
//! data/game/story/intro: the models, which of their animations play from when, their transforms and the camera's
//! sampled every frame, captions.
class StoryCutsceneDrawable : public Drawable
{
public:
   //! timeline: the cutscene's file in data/game, e.g. "story/intro/intro.txt"
   StoryCutsceneDrawable(RenderDevice& device, std::string timeline);
   ~StoryCutsceneDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float global_time) override;

   //! shows the cutscene and plays it, finished is called once it's over or skipped (right away if it can't be
   //! loaded); it stays visible and black until hidden
   void play(std::function<void()> finished);

   //! ends it early
   void skip();

   [[nodiscard]] bool isPlaying() const;

private:
   struct Action
   {
      //! seconds into the intro
      float _start = 0.0f;
      std::string _name;

      //! loops, or stops on its last frame
      bool _loop = true;

      int32_t _animation = -1;
      float _duration = 0.0f;
   };

   struct Actor
   {
      std::string _name;
      std::unique_ptr<GltfModel> _model;

      //! seconds into the intro it's seen in
      float _from = 0.0f;
      float _until = 0.0f;

      std::vector<Action> _actions;

      //! one per frame, or a single one for those that stay put
      std::vector<Matrix> _matrices;
   };

   //! text shown over the picture, fading in and out
   struct Caption
   {
      //! seconds into the cutscene
      float _from = 0.0f;
      float _until = 0.0f;
      std::string _text;
   };

   bool load();
   void finish();
   void drawCaptions();
   Matrix sample(const std::vector<Matrix>& matrices, float time) const;

   //! 0 black .. 1 fully shown, fading in at the start and out at the end
   float getFade() const;

   std::string _timeline;
   std::vector<Actor> _actors;
   std::vector<Caption> _captions;
   BitmapFont* _font = nullptr;
   std::vector<Matrix> _camera;

   //! the camera's horizontal field of view per frame, degrees
   std::vector<float> _fov;

   float _fps = 24.0f;
   float _duration = 0.0f;
   bool _loaded = false;

   //! lit by lamps indoors rather than by the moon
   bool _indoors = false;
   bool _load_failed = false;

   std::unique_ptr<GlowSprites> _glows;
   std::unique_ptr<NightSky> _sky;

   //! seconds into the intro, negative while it isn't playing
   float _time = -1.0f;
   float _last_time = 0.0f;
   std::function<void()> _finished;
};
