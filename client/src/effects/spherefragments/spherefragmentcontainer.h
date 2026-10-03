#pragma once

// engine
#include "math/vector.h"
#include "render/texture.h"

#include <cstdint>
#include <memory>
#include <vector>

class SphereFragment;
class Node;

class SphereFragmentContainer
{
public:
   explicit SphereFragmentContainer(const Node& root);
   virtual ~SphereFragmentContainer();

   void animate(float time);

   //! draw fragments
   void drawFragments(const Vector& camera_position);

   void begin();
   void end();

protected:
   std::vector<std::unique_ptr<SphereFragment>> _fragments;

   Texture _earth_texture{0};
   Texture _normal_map_texture{0};
   Texture _lava_map_texture{0};

   uint32_t _shader = 0;

   int32_t _size_param = -1;
   int32_t _project_matrix_param = -1;
   int32_t _model_matrix_param = -1;
   int32_t _fresnel_param = -1;
   int32_t _light_param = -1;
   int32_t _camera_param = -1;
   int32_t _texture_map_param = -1;
   int32_t _normal_map_param = -1;
   int32_t _specular_map_param = -1;
   int32_t _lava_map_param = -1;
};
