#pragma once

#include "math/matrix.h"

#include <memory>
#include <vector>

class Detonation;

// Real volumetric flame effect: 3D procedural noise volume + a gradient palette texture.
class DetonationManager
{
public:
   DetonationManager();
   ~DetonationManager();

   void init();
   void clear();
   void addDetonation(int x, int y, int top, int bottom, int left, int right);

   void update(float time);
   void render();

private:
   void drawExplosion(Detonation* det, float time);
   void drawBox(float x, float y, float z, float left, float right, float bottom, float top, int sides);

   float _time;
   unsigned int _shader;

   unsigned int _noise_map;
   unsigned int _gradient_map;

   int _param_time;
   int _param_cam_pos;
   int _param_top;
   int _param_bottom;
   int _param_left;
   int _param_right;
   int _param_bound_min;
   int _param_bound_max;
   int _param_noise_map;
   int _param_gradient_map;

   unsigned int _box_vertex_buffer;

   std::vector<std::unique_ptr<Detonation>> _detonations;
};
