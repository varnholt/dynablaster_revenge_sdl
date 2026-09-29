#pragma once

// engine
#include "math/matrix.h"
#include "math/vector.h"

#include <cstdint>
#include <vector>

class Mesh;
class Image;

class SphereFragment
{
public:
   SphereFragment(const std::vector<Mesh*>& meshes, const Image* order_image);

   int32_t getPartCount() const;

   void animate(float time, const Matrix& rotation);

   const Matrix* getMatrices() const;

   float* getFresnelFactors();

   //! draw single fragment
   void draw();

protected:
   uint32_t _vertex_buffer = 0;
   uint32_t _index_buffer = 0;
   int32_t _vertex_count = 0;
   int32_t _index_count = 0;
   std::vector<Matrix> _matrix;
   std::vector<float> _random;
   std::vector<float> _time;
   std::vector<Matrix> _model_view;
   std::vector<float> _fresnel_factors;
};
