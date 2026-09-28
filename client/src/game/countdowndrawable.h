#pragma once

// GLES3 port of client/src/game/countdowndrawable.h. Interface unchanged from the original
// except dropping _layer_ids (declared, never read or written anywhere in the original) and the
// unused <QImage> include.

#include "drawable.h"

#include <memory>
#include <string>
#include <vector>

#include "image/psd.h"

class PSDLayer;

class CountdownDrawable : public Drawable
{
public:
   CountdownDrawable(RenderDevice* dev);
   virtual ~CountdownDrawable();

   void initializeGL();
   void paintGL();
   void animate(float time);

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

   int _time_left;
   float _animation_start_time;
   bool _animation_active;
   float _delta_time;
   float _time;
   bool _delta_time_initialized;

   // GLES3 port addition - the shared per-item menu shader (see menus/defaultshader.h), bound
   // once per paintGL() call instead of the original's `_device->setShader(0)` (desktop GL's
   // fixed-function fallback, which GLES3 has no equivalent of).
   unsigned int _shader;
};
