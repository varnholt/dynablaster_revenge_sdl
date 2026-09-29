#pragma once

// GLES3 port of client/src/game/countdowndrawable.h. Interface unchanged from the original
// except dropping _layer_ids (declared, never read or written anywhere in the original) and the
// unused <QImage> include.

#include "drawable.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include "image/psd.h"

class PSDLayer;

class CountdownDrawable : public Drawable
{
public:
   CountdownDrawable(RenderDevice* dev);
   ~CountdownDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float time) override;

   void countdown(int left);

protected:
   void drawCountdown();
   void initializeLayers();
   void initGlParameters();
   void cleanupGlParameters();

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;
   std::vector<float> _layer_alphas;
   std::string _filename;

   int _time_left = 0;
   float _animation_start_time = -1.0f;
   bool _animation_active = false;
   float _delta_time = 0.0f;
   float _time = 0.0f;
   bool _delta_time_initialized = false;

   // GLES3 port addition - the shared per-item menu shader (see menus/defaultshader.h), bound
   // once per paintGL() call instead of the original's `_device->setShader(0)` (desktop GL's
   // fixed-function fallback, which GLES3 has no equivalent of).
   uint32_t _shader = 0;
};
