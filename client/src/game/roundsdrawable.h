#pragma once

#include "drawable.h"

#include <memory>
#include <string>
#include <vector>

#include "image/psd.h"

class MotionBlurFilter;
class PSDLayer;

class RoundsDrawable : public Drawable
{
public:
   RoundsDrawable(RenderDevice* dev);
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

   PSDLayer* _layer_round = nullptr;
   PSDLayer* _layer_round_final = nullptr;
   PSDLayer* _layer_round1 = nullptr;
   PSDLayer* _layer_round2 = nullptr;
   PSDLayer* _layer_round3 = nullptr;
   PSDLayer* _layer_round4 = nullptr;
   PSDLayer* _layer_round5 = nullptr;

   std::string _filename;

   float _time = 0.0f;
   float _start_time = 0.0f;
   bool _initialize_time = false;

   std::unique_ptr<MotionBlurFilter> _motion_blur_filter;

   float _max_layer_width = 0.0f;
};
