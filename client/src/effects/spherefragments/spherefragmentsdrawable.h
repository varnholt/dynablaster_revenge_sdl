#pragma once

// base
#include "framework/drawable.h"

#include "framework/globaltime.h"
#include "math/vector.h"

#include <memory>

class BombFuzeGeometryVbo;
class BombSocketGeometryVbo;
class BlurFilter;
class RenderDevice;
class SceneGraph;
class SphereFragmentContainer;
class SphereGeometryVbo;
class DuplicateAlpha;
class BlendQuad;
class FrameBuffer;

class SphereFragmentsDrawable : public Drawable
{
public:
   explicit SphereFragmentsDrawable(RenderDevice* device, bool visible = false);
   ~SphereFragmentsDrawable() override;

   void initializeGL() override;
   void paintGL() override;

   void removeFragments();

protected:
   //! setup project matrix
   void projectionSetup();

   //! draw bomb parts
   void drawBombParts();

   std::unique_ptr<SceneGraph> _scene_graph_earth;
   std::unique_ptr<SceneGraph> _scene_graph_bomb;

   // the vbos reference geometry owned by the scene graphs above
   std::unique_ptr<SphereGeometryVbo> _bomb;
   std::unique_ptr<BombFuzeGeometryVbo> _fuze;
   std::unique_ptr<BombSocketGeometryVbo> _socket;

   std::unique_ptr<SphereFragmentContainer> _fragment_container;
   std::unique_ptr<BlurFilter> _blur;
   std::unique_ptr<DuplicateAlpha> _alpha_duplicate;
   std::unique_ptr<BlendQuad> _blend_quad;

   Vector _camera;

   //! fading
   float _alpha = 1.0f;

   Vector _position_offset;
   float _scale = 0.65f;

   // render targets, sized to the main viewport lazily in paintGL()
   std::unique_ptr<FrameBuffer> _earth_fb;
   std::unique_ptr<FrameBuffer> _aura_fb;
   std::unique_ptr<FrameBuffer> _bomb_fb;
};
