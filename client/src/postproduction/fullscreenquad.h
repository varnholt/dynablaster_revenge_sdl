#pragma once

#include "gles3.h"

// shared position+texcoord quad drawn as two triangles (GLES3 has no immediate mode)
class FullScreenQuad
{
public:
   FullScreenQuad();

   // static unit quad spanning (-1,-1)-(1,1) at z=-1 with uv 0..1, for shaders that write
   // gl_Position straight from the position attribute
   void drawUnit();

   // axis-aligned rect from (x0,y0) to (x1,y1) with texcoords (u0,v0)-(u1,v1), uploaded into
   // a dynamic vertex buffer every call
   void drawRect(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);

private:
   GLuint _unit_buffer = 0;
   GLuint _dynamic_buffer = 0;
};
