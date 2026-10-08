#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"

//! a model from a binary glTF (.glb): textured meshes, optionally skinned and animated
//!
//! glTF is y-up, the game is z-up; the model is turned on load so its +z is up again.
class GltfModel
{
public:
   //! per draw call look
   struct Look
   {
      //! 0..1, whitens the model (hit flash)
      float _flash = 0.0f;

      //! 0..1, fades the model out
      float _alpha = 1.0f;

      //! multiplied onto the texture
      Vector _tint = Vector(1.0f, 1.0f, 1.0f);
   };

   explicit GltfModel(const std::string& filename);
   ~GltfModel();

   GltfModel(const GltfModel&) = delete;
   GltfModel& operator=(const GltfModel&) = delete;

   [[nodiscard]] bool isValid() const;

   //! index of the animation with that name, -1 if there's none
   [[nodiscard]] int32_t findAnimation(std::string_view name) const;

   //! seconds
   [[nodiscard]] float getAnimationDuration(int32_t animation) const;

   //! bind pose bounds, z-up
   [[nodiscard]] const Vector& getMinimum() const;
   [[nodiscard]] const Vector& getMaximum() const;

   //! uploads the gpu resources, needs the gl context
   void initializeGL();

   //! draws the model posed by the animation at time (seconds, wrapped), no animation for -1
   void draw(const Matrix& transform, int32_t animation, float time, const Look& look);

   //! the shader and its state for a batch of draw() calls
   static void begin();
   static void end();

private:
   using Matrix4 = std::array<float, 16>;

   struct Vertex
   {
      float _position[3];
      float _normal[3];
      float _uv[2];
      float _joints[4];
      float _weights[4];
   };

   struct Primitive
   {
      int32_t _node = -1;
      bool _skinned = false;
      std::vector<Vertex> _vertices;
      std::vector<uint32_t> _indices;
      int32_t _color_image = -1;
      int32_t _occlusion_image = -1;
      uint32_t _vertex_buffer = 0;
      uint32_t _index_buffer = 0;
   };

   struct Node
   {
      int32_t _parent = -1;
      std::array<float, 3> _translation{0.0f, 0.0f, 0.0f};
      std::array<float, 4> _rotation{0.0f, 0.0f, 0.0f, 1.0f};
      std::array<float, 3> _scale{1.0f, 1.0f, 1.0f};
   };

   struct Channel
   {
      int32_t _node = -1;

      //! 0 translation, 1 rotation, 2 scale
      int32_t _path = 0;

      bool _step = false;
      std::vector<float> _times;
      std::vector<float> _values;
   };

   struct Animation
   {
      std::string _name;
      float _duration = 0.0f;
      std::vector<Channel> _channels;
   };

   struct Image
   {
      int32_t _width = 0;
      int32_t _height = 0;
      std::vector<uint8_t> _pixels;
      uint32_t _texture = 0;
   };

   void load(const std::string& filename);
   void pose(int32_t animation, float time, std::vector<Matrix4>& globals) const;

   bool _valid = false;
   bool _gl_initialized = false;
   std::vector<Node> _nodes;
   std::vector<Primitive> _primitives;
   std::vector<Animation> _animations;
   std::vector<Image> _images;
   std::vector<int32_t> _joints;
   std::vector<Matrix4> _inverse_bind;
   Vector _minimum;
   Vector _maximum;
};
