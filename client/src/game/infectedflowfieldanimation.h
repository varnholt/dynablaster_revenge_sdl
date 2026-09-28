#pragma once

#include "math/vector.h"
#include "math/vector4.h"

class InfectedFlowFieldAnimation
{
public:
   InfectedFlowFieldAnimation();
   ~InfectedFlowFieldAnimation();

   void initialize();
   void initializePositions(unsigned int depth_map, const Vector& min, const Vector& max);
   void initializeParams(unsigned int depth_map, const Vector& min, const Vector& max);

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
   bool _initialized;
   int _width;
   int _height;
   unsigned int _vertex_pos_buffer;
   unsigned int _vertex_color_buffer;
   unsigned int _vertex_param_texture;
   unsigned int _param_target;
   unsigned int _vertex_color_texture;
   unsigned int _color_target;
   unsigned int _positions[2];
   unsigned int _pos_target[2];
   int _page;

   Vector _center;
   float _flow_scale;
   float _particle_size;

   float _elapsed;
   bool _stop;
};
