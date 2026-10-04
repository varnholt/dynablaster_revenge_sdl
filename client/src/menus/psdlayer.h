#pragma once

#include "image/psd.h"
#include "math/matrix.h"
#include "render/texture.h"

#include <cstdint>
#include <functional>

/// \brief renders one PSD layer as a textured quad through whichever shader the caller has bound
/// (normally the shared menu shader, see defaultshader.h); opacity goes into its "alpha" uniform.
class PSDLayer
{
public:
   struct Vertex
   {
      Vertex() = default;
      Vertex(float x, float y, float z, float u, float v) : x(x), y(y), z(z), u(u), v(v)
      {
      }

      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
      float u = 0.0f;
      float v = 0.0f;
   };

   PSDLayer(PSD::Layer& layer, float z = -1.0f, bool unwrap = true);
   virtual ~PSDLayer() = default;

   PSD::Layer& getLayer() const;
   uint32_t getTexture() const;
   uint32_t getVertexBuffer() const;
   uint32_t getIndexBuffer() const;
   float getOpacity() const;
   void setOpacity(float opacity);

   void render(float x = 0.0f, float y = 0.0f, float alpha = 1.0f);

   //! renders the layer's quad (0,0)-(width,height) through  world instead of its PSD position
   void render(const Matrix& world, float alpha);

   float getU() const;
   float getV() const;

   int getWidth() const;
   int getHeight() const;
   int getLeft() const;
   int getRight() const;
   int getTop() const;
   int getBottom() const;

private:
   // the layer belongs to its PSD
   std::reference_wrapper<PSD::Layer> _layer;
   Texture _texture;
   uint32_t _vertex_buffer = 0;
   uint32_t _index_buffer = 0;
   float _opacity = 0.0f;
   float _u = 0.0f;
   float _v = 0.0f;
};
