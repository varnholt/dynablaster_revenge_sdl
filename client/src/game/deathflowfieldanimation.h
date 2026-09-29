#pragma once

/// \brief GLES3 port of client/src/game/deathflowfieldanimation.cpp - see project memory for the
/// full GPGPU-particle-simulation background. Interface unchanged from the original.

#include "math/vector.h"
#include "math/vector4.h"
#include <cstdint>

class FrameBuffer;

class DeathFlowFieldAnimation
{
public:
   DeathFlowFieldAnimation();
   ~DeathFlowFieldAnimation();

   // initialize flowfield from 2d rendering
   void initialize(FrameBuffer* src, const Vector& min, const Vector& max);
   void initializePositions(uint32_t depth_map, const Vector& min, const Vector& max);
   void initializeParams(uint32_t depth_map, const Vector& min, const Vector& max);

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

   uint32_t getColorMap() const;

   bool isElapsed() const;

private:
   int _width = 0;
   int _height = 0;
   uint32_t _vertex_pos_buffer = 0;
   uint32_t _vertex_uv_buffer = 0;
   uint32_t _vertex_params = 0;
   uint32_t _positions[2];
   uint32_t _target[2];
   uint32_t _texture = 0;
   int _page = 0;

   Vector _center;
   float _flow_scale = 0.2f;
   float _particle_size = 0.0f;

   float _elapsed = 0.0f;
};
