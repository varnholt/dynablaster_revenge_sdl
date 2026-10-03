#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "math/matrix.h"

class SceneGraph;
class Node;
class Mesh;

// per frame: one matrix per bone
using BoneAnimPrecalc = std::vector<std::vector<Matrix>>;

class MotionMixer
{
public:
   MotionMixer();
   ~MotionMixer();

   static void cleanup();
   static int32_t addAnimation(const std::string& name);
   static std::optional<std::reference_wrapper<Mesh>> getMesh(const std::string& name);

   void setAnimation(int32_t anim1, int32_t anim2, float weight1, int32_t anim3, float weight2);
   void animate(float frame);
   Node& getNode(int32_t index) const;

private:
   static std::unique_ptr<SceneGraph> _reference_animation;
   static std::vector<BoneAnimPrecalc> _matrix_precalc;

   static float _frame_step;
   static int32_t _bone_count;
   static int32_t _animation_length;

   std::vector<std::unique_ptr<Node>> _current_state;

   int32_t _anim1 = -1;
   int32_t _anim2 = -1;
   int32_t _anim3 = -1;
   float _weight1 = 1.0f;
   float _weight2 = 0.0f;
};
