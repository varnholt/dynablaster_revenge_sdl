#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"
#include "math/vector2.h"

class FrameBuffer;
class GltfModel;

//! water with waves, reflecting what stands around it and refracting the ground below it;
//! per frame: renderReflection(), the opaque scene, captureRefraction(), render()
class WaterSurface
{
public:
   //! lava: molten rock instead of water (lava-frag.glsl), lit by itself, no mirror image or refraction needed
   explicit WaterSurface(bool lava = false);

   [[nodiscard]] bool isLava() const;
   ~WaterSurface();

   WaterSurface(const WaterSurface&) = delete;
   WaterSurface& operator=(const WaterSurface&) = delete;

   //! draws the mirror image: draw() gets the transform that mirrors at the water level and draws what
   //! should show in the water with GltfModel (whose clip height drops what ends up above the water)
   void renderReflection(float level, const std::function<void(const Matrix&)>& draw);

   //! keeps what has been drawn so far, the water shows it through its surface
   void captureRefraction();

   //! someone standing in the water: where (world xy) and how fast it walks (tiles a second)
   struct Wader
   {
      Vector2 _position;
      Vector2 _velocity;
   };

   //! as many as the shader takes, the rest wade without foam
   static constexpr size_t MAX_WADERS = 16;

   //! the foam ring round their feet, the ripples running off them and their wake on the next render()
   void setWaders(std::vector<Wader> waders);

   //! draws the model's water surfaces seen from eye
   void render(GltfModel& model, const Vector& eye);

private:
   uint32_t _shader = 0;
   int32_t _data_param = -1;
   int32_t _reflection_param = -1;
   int32_t _refraction_param = -1;
   int32_t _time_param = -1;
   int32_t _eye_param = -1;
   int32_t _deep_param = -1;
   int32_t _origin_param = -1;
   int32_t _size_param = -1;
   int32_t _haze_param = -1;
   int32_t _flow_param = -1;
   int32_t _haze_box_param = -1;
   int32_t _haze_range_param = -1;
   int32_t _waders_param = -1;
   int32_t _wader_count_param = -1;

   std::vector<Wader> _waders;

   std::unique_ptr<FrameBuffer> _reflection;
   uint32_t _refraction = 0;

   bool _lava = false;
};
