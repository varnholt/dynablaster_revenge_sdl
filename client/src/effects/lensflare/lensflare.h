#pragma once

#include "math/vector.h"
#include "math/vector2.h"
#include "render/texture.h"

#include <cstdint>
#include <string>
#include <vector>

// one flare setup: a strip of textured ghosts laid out along the ray from the sun's screen
// position through the screen center
class LensFlare
{
public:
   struct Config
   {
      std::string texture;
      std::string ghosts;
      Vector sun_3d;
      float max_length = 0.0f;
      float lower_limit = 0.0f;
      float angle = 0.0f;
      bool inverted = false;
      float invert_offset = 0.0f;
      bool sun_is_2d = false;
   };

   explicit LensFlare(const Config& config);
   ~LensFlare();

   LensFlare(const LensFlare&) = delete;
   LensFlare& operator=(const LensFlare&) = delete;

   const Config& getConfig() const;

   //! \param sun_2d sun position in the flare's ortho space
   void draw(const Vector2& sun_2d, int32_t time_param, int32_t sun_param, int32_t length_param);

private:
   struct Ghost
   {
      float ray_position = 0.5f;
      float offset_y = -0.05f;
      int32_t tile_column = 0;
      int32_t tile_row = 0;
      float size = 0.2f;
      Vector color = Vector(1.0f, 1.0f, 1.0f);
      int32_t width = 1;
      int32_t height = 1;
      float angle = 0.0f;
      float speed = 0.0f;
   };

   static std::vector<Ghost> readGhostStrip(const std::string& filename);
   void createBuffers(const std::vector<Ghost>& ghosts);

   Config _config;
   Texture _texture;
   unsigned int _vertex_buffer = 0;
   unsigned int _index_buffer = 0;
   int32_t _index_count = 0;
};
