#include "motionmixer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include "math/matrix.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "nodes/scenegraph.h"

std::unique_ptr<SceneGraph> MotionMixer::_reference_animation;
std::vector<BoneAnimPrecalc> MotionMixer::_matrix_precalc;
float MotionMixer::_frame_step = 50.0f;
int32_t MotionMixer::_bone_count = 0;
int32_t MotionMixer::_animation_length = 0;

MotionMixer::MotionMixer()
{
   _current_state.reserve(_bone_count);
   for (int32_t i = 0; i < _bone_count; i++)
   {
      _current_state.push_back(std::make_unique<Node>(Node::idDummy));
   }
}

MotionMixer::~MotionMixer() = default;

// final cleanup of static data
void MotionMixer::cleanup()
{
   _reference_animation.reset();
   _matrix_precalc.clear();
}

namespace
{
// find the node in the tree below "tree_root" that has the same name path as "source"
std::optional<std::reference_wrapper<Node>> matchTreeNode(Node& tree_root, const Node& source)
{
   std::vector<std::string> name_list;
   for (std::optional<std::reference_wrapper<const Node>> node = source; node; node = node->get().parent())
   {
      name_list.push_back(node->get().name());
   }

   std::optional<std::reference_wrapper<Node>> match = tree_root;
   for (int32_t i = static_cast<int32_t>(name_list.size()) - 2; i >= 0 && match;)
   {
      Node& current = *match;
      if (const auto child = current.findChild(name_list[i]))
      {
         match = child;
         i--;
      }
      else if (current.getChildCount() > 0)
      {
         match = current.getChild(0);
      }
      else
      {
         match.reset();
      }
   }

   return match;
}
}  // namespace

int32_t MotionMixer::addAnimation(const std::string& name)
{
   const int32_t index = static_cast<int32_t>(_matrix_precalc.size());

   // the first loaded animation is kept as reference, all others are dropped after precalc
   auto owned_scene = std::make_unique<SceneGraph>();
   SceneGraph& scene = *owned_scene;
   scene.load(name);

   if (!_reference_animation)
   {
      _reference_animation = std::move(owned_scene);
   }

   // match nodes to reference animation
   for (const auto& reference_node : _reference_animation->nodeList())
   {
      if (!matchTreeNode(scene, *reference_node))
      {
         std::printf("node match fail!\n");
      }
   }

   // find required bone nodes
   std::vector<std::reference_wrapper<Bone>> bone_list;  // bone initial transform
   for (int32_t i = 0; i < scene.size(); i++)
   {
      Node& node = scene.getNode(i);
      const int32_t node_animation_length = node.getAnimationLength();
      if (node_animation_length > _animation_length)
      {
         _animation_length = node_animation_length;
      }

      if (node.id() == Node::idMesh)
      {
         const auto& mesh = static_cast<const Mesh&>(node);
         for (int32_t p = 0; p < mesh.getPartCount(); p++)
         {
            Geometry& geometry = mesh.getPart(p);
            for (Bone& bone : geometry.getBoneList())
            {
               bone_list.emplace_back(bone);
            }
         }
      }
   }

   const int32_t frames = static_cast<int32_t>(std::ceil(_animation_length / _frame_step));

   _bone_count = std::max(_bone_count, static_cast<int32_t>(bone_list.size()));

   // adjust length of existing animations
   for (BoneAnimPrecalc& precalc : _matrix_precalc)
   {
      const std::vector<Matrix> last = precalc.back();
      while (static_cast<int32_t>(precalc.size()) < frames)
      {
         precalc.push_back(last);
      }
   }

   // precalc animation matrices for relevant nodes
   BoneAnimPrecalc precalc;
   precalc.reserve(frames + 1);

   for (int32_t fr = 0; fr <= frames; fr++)
   {
      std::vector<Matrix> animation;
      animation.reserve(bone_list.size());

      const float frame = fr * _frame_step;

      // transform all scene nodes
      for (const auto& node : scene.nodeList())
      {
         node->transform(frame);
      }

      // store relevant node matrices
      for (const Bone& bone : bone_list)
      {
         animation.push_back(bone.transform() * scene.getNode(bone.id()).getTransform());
      }
      precalc.push_back(std::move(animation));
   }
   _matrix_precalc.push_back(std::move(precalc));

   // fix bone ids
   for (size_t i = 0; i < bone_list.size(); i++)
   {
      bone_list[i].get().setId(static_cast<int32_t>(i));
   }

   return index;
}

void MotionMixer::setAnimation(int32_t anim1, int32_t anim2, float weight1, int32_t anim3, float weight2)
{
   _anim1 = anim1;
   _anim2 = anim2;
   _anim3 = anim3;
   _weight1 = weight1;
   _weight2 = weight2;
}

void MotionMixer::animate(float frame)
{
   if (_anim1 < 0 || _anim2 < 0)
   {
      return;
   }

   const BoneAnimPrecalc& anim1 = _matrix_precalc[_anim1];
   const BoneAnimPrecalc& anim2 = _matrix_precalc[_anim2];

   frame /= 50.0f;
   if (frame < 0.0)
   {
      frame = 0.0f;
   }
   int32_t index = static_cast<int32_t>(std::floor(frame));
   const int32_t last_blendable = static_cast<int32_t>(anim1.size()) - 2;
   if (index > last_blendable)
   {
      index = last_blendable;
   }
   const float t = (index + 1) - frame;

   const std::vector<Matrix>& a11 = anim1[index];
   const std::vector<Matrix>& a12 = anim1[index + 1];

   const std::vector<Matrix>& a21 = anim2[index];
   const std::vector<Matrix>& a22 = anim2[index + 1];

   for (size_t i = 0; i < _current_state.size(); i++)
   {
      const Matrix m1 = Matrix::blend(a11[i], a12[i], t);
      const Matrix m2 = Matrix::blend(a21[i], a22[i], t);
      _current_state[i]->setTransform(Matrix::blend(m1, m2, _weight1));
   }

   if (_weight2 > 0.0f && _anim3 >= 0)
   {
      const BoneAnimPrecalc& anim3 = _matrix_precalc[_anim3];

      const std::vector<Matrix>& a31 = anim3[index];
      const std::vector<Matrix>& a32 = anim3[index + 1];

      for (size_t i = 0; i < _current_state.size(); i++)
      {
         const Matrix& m1 = _current_state[i]->getTransform();
         const Matrix m3 = Matrix::blend(a31[i], a32[i], t);
         _current_state[i]->setTransform(Matrix::blend(m1, m3, _weight2));
      }
   }
}

std::optional<std::reference_wrapper<Mesh>> MotionMixer::getMesh(const std::string& name)
{
   if (_reference_animation)
   {
      return _reference_animation->findNode<Mesh>(name);
   }
   return std::nullopt;
}

Node& MotionMixer::getNode(int32_t index) const
{
   return *_current_state[index];
}
