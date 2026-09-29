#pragma once

#include "math/matrix.h"

#include <cstdint>
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

   float _time = 0.0f;
   uint32_t _shader = 0;

   uint32_t _noise_map = 0;
   uint32_t _gradient_map = 0;

   int _param_time = 0;
   int _param_cam_pos = 0;
   int _param_top = 0;
   int _param_bottom = 0;
   int _param_left = 0;
   int _param_right = 0;
   int _param_bound_min = 0;
   int _param_bound_max = 0;
   int _param_noise_map = 0;
   int _param_gradient_map = 0;

   uint32_t _box_vertex_buffer = 0;

   std::vector<std::unique_ptr<Detonation>> _detonations;
};
