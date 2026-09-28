#pragma once

// Renders the earth/bomb/fragments effect, the "dynablaster"/"revenge" PSD text overlays, and the
// spark point sprites. Dead cube/earth-highlight subsystems are not ported (unused in the original).

#include "effects/spherefragments/spherefragmentsdrawable.h"

// engine
#include "image/psd.h"
#include "math/vector.h"
#include "tools/array.h"

#include <string>

class PSDLayer;

class GameLogoDrawable : public SphereFragmentsDrawable
{
public:
   class Spark
   {
   public:
      Spark() : _point_size(1.0f), _intensity(1.0f), _length(0.0f), _scalar(0.0f), _start_time(0.0f)
      {
      }

      Vector _origin;
      Vector _direction;
      Vector _position;

      float _point_size;
      float _intensity;
      float _length;
      float _scalar;
      float _start_time;
   };

   //! constructor
   GameLogoDrawable(RenderDevice* dev, bool visible = false);

   //! destructor
   virtual ~GameLogoDrawable();

   //! initialize
   void initializeGL();

   //! draw
   void paintGL();

   //! animate
   void animate(float time);

   //! overwrite base
   void setVisible(bool visible);

   //!
   void pageChanged(const std::string& page);

protected:
   //! initialize layers
   void initializeLayers();

   //! initialize sparks
   void initializeSparks();

   //! init ortho gl parameters
   void initOrthoGlParameters();

   //! init pointsprite gl parameters
   void initPointSpriteGlParameters();

   //! cleanup gl parameters
   void cleanupGlParameters();

   // main drawing

   //! set alpha for psd layers
   void updateFadeAlpha();

   //! draw point sprites
   void drawSparks();

   //
   void initSpark(Spark& spark);

   //
   bool _main_menu_visible;

   // animation

   //! time
   float _delta_time;
   float _time;

   // overlay members

   //! psd instance
   PSD _psd;

   //! filename to load from
   std::string _filename;

   //! font texture
   PSDLayer* _layer_dynablaster;

   //! font texture
   PSDLayer* _layer_revenge;

   //! all layers (owns them - see destructor)
   Array<PSDLayer*> _layers;

   //
   float _fade_in_end;
   float _fade_out_end;

   // spark point sprites

   Array<Spark> _sparks;
   Vector _spark_origin;
   bool _spark_times_initialized;
};
