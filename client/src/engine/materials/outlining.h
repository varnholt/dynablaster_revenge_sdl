// draws outlines

#pragma once

#include <cstdint>
#include "material.h"
#include "math/matrix.h"
#include "math/vector4.h"

class Outlining : public Material
{
public:
   Outlining();
   void load(Stream& stream) override;
   void add(Geometry& geometry);
   void update(float frame, const Matrix& camera) override;
   void renderDiffuse() override;

private:
   void init() override;
   void begin() override;
   void end() override;
   int32_t calcEdgeIndices(uint32_t index_buffer, const Geometry& geometry, const Vector& viewer);

   uint32_t _shader = 0;
   int32_t _param_color = -1;

   Matrix _camera;
};
