#pragma once

#include "drawable.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "image/psd.h"

class MotionBlurFilter;
class PSDLayer;

class RoundsDrawable : public Drawable
{
public:
   explicit RoundsDrawable(RenderDevice& dev);
   ~RoundsDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float time) override;

   void showGame();

protected:
   void initializeLayers();
   void initGlParameters();
   void cleanupGlParameters();
   float getRelativeTime() const;

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;

   std::optional<std::reference_wrapper<PSDLayer>> _layer_round;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_round_final;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_round1;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_round2;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_round3;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_round4;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_round5;

   std::string _filename;

   float _time = 0.0f;
   float _start_time = 0.0f;
   bool _initialize_time = false;

   std::unique_ptr<MotionBlurFilter> _motion_blur_filter;

   float _max_layer_width = 0.0f;
};
