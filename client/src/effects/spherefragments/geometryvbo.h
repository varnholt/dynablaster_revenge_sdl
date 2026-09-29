#pragma once

#include "vertex3d.h"

#include <cstdint>

class Geometry;

class GeometryVbo
{
public:
   explicit GeometryVbo(Geometry* geometry);
   virtual ~GeometryVbo() = default;

   virtual void initialize();

protected:
   // binds the vbo/ibo and draws position, normal and uv
   void drawGeometry();

   uint32_t _vertex_buffer = 0;
   uint32_t _index_buffer = 0;

   Geometry* _geometry = nullptr;
};
