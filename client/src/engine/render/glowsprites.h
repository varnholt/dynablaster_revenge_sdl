#pragma once

#include <cstdint>
#include <vector>

#include "engine/gltf/gltfmodel.h"
#include "math/matrix.h"

//! halos of light sources: camera facing, additive, hidden by what stands in front of them;
//! they flicker, blink or pulse by their mode (see GltfModel::Glow)
class GlowSprites
{
public:
   GlowSprites();
   ~GlowSprites();

   GlowSprites(const GlowSprites&) = delete;
   GlowSprites& operator=(const GlowSprites&) = delete;

   //! replaces the lights, they are in the space of the transform given to render()
   void set(const std::vector<GltfModel::Glow>& glows);

   //! draws them with the current camera, after the opaque geometry around them
   void render(const Matrix& transform);

private:
   uint32_t _shader = 0;
   int32_t _time_param = -1;
   int32_t _aspect_param = -1;
   uint32_t _vertex_buffer = 0;
   int32_t _vertex_count = 0;
};
