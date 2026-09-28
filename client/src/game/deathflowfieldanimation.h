#pragma once

/// \brief GLES3 port of client/src/game/deathflowfieldanimation.cpp - see project memory for the
/// full GPGPU-particle-simulation background. Interface unchanged from the original.

#include "math/vector.h"
#include "math/vector4.h"

class FrameBuffer;

class DeathFlowFieldAnimation
{
public:
   DeathFlowFieldAnimation();
   ~DeathFlowFieldAnimation();

   // initialize flowfield from 2d rendering
   void initialize(FrameBuffer* src, const Vector& min, const Vector& max);
   void initializePositions(unsigned int depth_map, const Vector& min, const Vector& max);
   void initializeParams(unsigned int depth_map, const Vector& min, const Vector& max);

   // center reference position (world space 3d)
   const Vector& getCenter() const;
   void setCenter(const Vector& center);

   // flowfield scale
   float getScale() const;
   void setScale(float scale);

   //! return size of particles in unprojected units
   float getPointSize() const;

   // update vertex position from flowfield directions
   void update(float delta_time);

   // draw particles
   void draw();

   unsigned int getColorMap() const;

   bool isElapsed() const;

private:
   int _width;
   int _height;
   unsigned int _vertex_pos_buffer;
   unsigned int _vertex_uv_buffer;
   unsigned int _vertex_params;
   unsigned int _positions[2];
   unsigned int _target[2];
   unsigned int _texture;
   int _page;

   Vector _center;
   float _flow_scale;
   float _particle_size;

   float _elapsed;
};
