#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/vector4.h"

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

   //! the air and light a scene sits in, from its extras: "atmosphere": [haze r, g, b, strength, near, far,
   //! ambient r, g, b, rim r, g, b]; the land fades into the haze from near to far units outside the box
   //! (usually the field), the ambient light and the rim light at the silhouettes take their colours
   struct Atmosphere
   {
      Vector _haze = Vector(0.0f, 0.0f, 0.0f);

      //! 0 for none
      float _strength = 0.0f;
      float _near = 0.0f;
      float _far = 1.0f;

      //! x0, y0, x1, y1 in world space
      Vector4 _box = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
      Vector _ambient = Vector(1.0f, 1.0f, 1.0f);
      Vector _rim = Vector(1.0f, 1.0f, 1.0f);
   };

   //! a light's glow, from a node's extras: "glow": [r, g, b, size, mode]
   struct Glow
   {
      //! model space, z-up, in the bind pose
      Vector _position;
      Vector _color;

      //! radius of the halo
      float _size = 0.5f;

      //! 0 steady, 1 flickering flame, 2 blinking, 3 pulsing, 4 a fire: an animated flame standing on the
      //! position, its halo flickering with it, 6 a firefly: wandering round the position, glowing up now and then
      int32_t _mode = 0;
   };

   static constexpr int32_t GlowFire = 4;

   explicit GltfModel(const std::string& filename);
   ~GltfModel();

   GltfModel(const GltfModel&) = delete;
   GltfModel& operator=(const GltfModel&) = delete;

   [[nodiscard]] bool isValid() const;

   //! index of the animation with that name, -1 if there's none
   [[nodiscard]] int32_t findAnimation(std::string_view name) const;

   //! index of the first node with that name, -1 if there's none
   [[nodiscard]] int32_t findNode(std::string_view name) const;

   //! seconds
   [[nodiscard]] float getAnimationDuration(int32_t animation) const;

   //! how far a walk carries the model (model units), measured from its feet, the joints whose names start
   //! with prefix: a foot carrying the model moves backwards under it as fast as the model moves forwards,
   //! so the backward sweep of the carrying feet is the ground covered. One value per sample, evenly spread
   //! over the cycle and its end, from 0 to the stride; empty if no vertex follows such a joint
   [[nodiscard]] std::vector<float> measureWalk(int32_t animation, std::string_view prefix, int32_t samples) const;

   //! bind pose bounds, z-up
   [[nodiscard]] const Vector& getMinimum() const;
   [[nodiscard]] const Vector& getMaximum() const;

   //! the lights placed in the model
   [[nodiscard]] const std::vector<Glow>& getGlows() const;

   //! the scene's atmosphere, neutral if it has none (its box is left to the caller)
   [[nodiscard]] const Atmosphere& getAtmosphere() const;

   //! uploads the gpu resources, needs the gl context
   void initializeGL();

   //! draws the model posed by the animation at time (seconds, wrapped), no animation for -1; only the
   //! meshes of the node root (see findNode()) and its children if root isn't -1, so one file can hold
   //! variants of a piece side by side
   void draw(const Matrix& transform, int32_t animation, float time, const Look& look, int32_t root = -1);

   //! the shader and its state for a batch of draw() calls
   static void begin();
   static void end();

   //! draws() skip fragments above this height (world z), for mirror images; a huge value turns it off
   static void setClipHeight(float height);

   //! 1 draws the colour textures, 0 the primitives' night textures (where they have one), between blends them
   static void setDaylight(float daylight);
   [[nodiscard]] static float getDaylight();

   //! the atmosphere draw() and the water draw in, until set otherwise; a default one is neutral
   static void setAtmosphere(const Atmosphere& atmosphere);
   [[nodiscard]] static const Atmosphere& getCurrentAtmosphere();

   //! the water surfaces, primitives of nodes with "water": [r, g, b] (the deep colour) in their extras;
   //! draw() leaves them out, drawWater() draws them with the shader that's set, texture unit 0 holds
   //! their texture, attributes 0..2 are position, normal and uv
   [[nodiscard]] bool hasWater() const;
   [[nodiscard]] float getWaterLevel() const;
   [[nodiscard]] const Vector& getWaterColor() const;

   //! units a second the water streams past along x (a ship under way), 0 for still water: "water": [r, g, b, flow]
   [[nodiscard]] float getWaterFlow() const;
   //! its water is lava (extras "lava" instead of "water")
   [[nodiscard]] bool isLava() const;
   void drawWater(const Matrix& transform);

private:
   using Matrix4 = std::array<float, 16>;

   struct Vertex
   {
      float _position[3];
      float _normal[3];
      float _uv[2];
      float _joints[4];
      float _weights[4];

      //! cloth (sails, flags) waving in the wind, direction times amplitude: for nodes with "wave": [amplitude]
      //! in their extras from their vertices' first colour, its rgb the direction (z-up, 0..1 for -1..1), its
      //! alpha how far; draw() moves them by its time
      float _wave[4];
   };

   struct Primitive
   {
      int32_t _node = -1;
      bool _skinned = false;
      bool _water = false;
      std::vector<Vertex> _vertices;
      std::vector<uint32_t> _indices;
      int32_t _color_image = -1;
      int32_t _occlusion_image = -1;

      //! the colour by night, from the material's emissive texture, blended in by setDaylight()
      int32_t _night_image = -1;

      //! fine surface detail (grass, gravel, bark) tiled over the colour in world space, from the material's
      //! extras "screen": [dark rgb, bright rgb]: a screen showing live static between the two colours, unlit
      bool _screen = false;
      Vector _screen_dark;
      Vector _screen_bright;

      //! extras "detail": [mask, image 0..3, repeats per unit 0..3, strength]: up to four detail images, the
      //! mask's rgba (read at the colour's uvs) how much of each shows where; 0 strength for none
      int32_t _detail_mask_image = -1;
      std::array<int32_t, 4> _detail_images{-1, -1, -1, -1};
      Vector4 _detail_scales;
      float _detail_strength = 0.0f;

      //! cutouts (grass, leaves) from the material's alphaMode "MASK": fragments whose colour's alpha is below
      //! this are dropped, 0 for opaque materials
      float _alpha_cutoff = 0.0f;

      //! a cutout material's doubleSided: both sides drawn, the back lit with the normal flipped; opaque materials
      //! keep their back faces culled, Blender's exporter marks every material double sided unless told otherwise
      bool _double_sided = false;

      //! extras "fur": [length, strands per unit]: fur grows where the colour's alpha is set, drawn as shells
      //! pushed out along the normals up to length; 0 length for none
      Vector2 _fur;
      uint32_t _vertex_buffer = 0;
      uint32_t _index_buffer = 0;
   };

   struct Node
   {
      std::string _name;
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

   //! node is root or one of its children, grandchildren, ...
   [[nodiscard]] bool isPartOf(int32_t node, int32_t root) const;
   void pose(int32_t animation, float time, std::vector<Matrix4>& globals) const;

   bool _valid = false;
   bool _gl_initialized = false;
   std::vector<Node> _nodes;
   std::vector<Primitive> _primitives;
   std::vector<Animation> _animations;
   std::vector<Image> _images;
   std::vector<int32_t> _joints;
   std::vector<Matrix4> _inverse_bind;
   std::vector<Glow> _glows;
   Atmosphere _atmosphere;
   bool _has_water = false;
   float _water_level = 0.0f;
   Vector _water_color;
   float _water_flow = 0.0f;
   bool _lava = false;
   Vector _minimum;
   Vector _maximum;
};
