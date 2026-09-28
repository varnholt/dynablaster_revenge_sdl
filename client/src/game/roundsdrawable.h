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
   virtual ~RoundsDrawable();

   void initializeGL();
   void paintGL();
   void animate(float time);

   void showGame();

protected:
   void initializeLayers();
   void initGlParameters();
   void cleanupGlParameters();
   float getRelativeTime() const;

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;

   PSDLayer* _layer_round;
   PSDLayer* _layer_round_final;
   PSDLayer* _layer_round1;
   PSDLayer* _layer_round2;
   PSDLayer* _layer_round3;
   PSDLayer* _layer_round4;
   PSDLayer* _layer_round5;

   std::string _filename;

   float _time;
   float _start_time;
   bool _initialize_time;

   MotionBlurFilter* _motion_blur_filter;

   float _max_layer_width;
};
