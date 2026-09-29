#pragma once

#include "math/vector.h"
#include "math/vector4.h"
#include <cstdint>

class InfectedFlowFieldAnimation
{
public:
   InfectedFlowFieldAnimation();
   ~InfectedFlowFieldAnimation();

   void initialize();
   void initializePositions(uint32_t depth_map, const Vector& min, const Vector& max);
   void initializeParams(uint32_t depth_map, const Vector& min, const Vector& max);

   bool initialized() const;
   void setInitialized(bool init);

   const Vector& getCenter() const;
   void setCenter(const Vector& center);

   float getScale() const;

   //! return size of particles in unprojected units
   float getPointSize() const;

   void updatePositions(float deltaTime);
   void updateColors();

   void draw();

   //! all particles have faded away
   bool isElapsed() const;

   //! stop respawning of particles
   void stop();
   bool stopped() const;

private:
   bool _initialized = false;
   int _width = 0;
   int _height = 0;
   uint32_t _vertex_pos_buffer = 0;
   uint32_t _vertex_color_buffer = 0;
   uint32_t _vertex_param_texture = 0;
   uint32_t _param_target = 0;
   uint32_t _vertex_color_texture = 0;
   uint32_t _color_target = 0;
   uint32_t _positions[2];
   uint32_t _pos_target[2];
   int _page = 0;

   Vector _center;
   float _flow_scale = 0.2f;
   float _particle_size = 0.0f;

   float _elapsed = 0.0f;
   bool _stop = false;
};
