#include "lensflare.h"

#include "framework/globaltime.h"
#include "gldevice.h"
#include "render/texturepool.h"

#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <optional>
#include <ranges>
#include <string_view>

namespace
{
// the ghost texture is a 4x4 tile atlas
constexpr int32_t TILE_COLUMNS = 4;
constexpr int32_t TILE_ROWS = 4;

const std::string FLARE_DIRECTORY = "data/effects/lensflare/flares/";

struct LensFlareVertex
{
   // corner offset, position along the ray, rotation speed
   float x = 0.0f;
   float y = 0.0f;
   float ray_position = 0.0f;
   float rotation_speed = 0.0f;
   // perpendicular offset (only y is used), uv
   float offset_x = 0.0f;
   float offset_y = 0.0f;
   float u = 0.0f;
   float v = 0.0f;
   // ghost color
   float r = 0.0f;
   float g = 0.0f;
   float b = 0.0f;
};

std::string_view trim(std::string_view text)
{
   const auto first = text.find_first_not_of(" \t\r");
   if (first == std::string_view::npos)
   {
      return {};
   }

   const auto last = text.find_last_not_of(" \t\r");
   return text.substr(first, last - first + 1);
}

template <typename T>
std::optional<T> parse(std::string_view text, int32_t base = 10)
{
   text = trim(text);
   T value{};
   std::from_chars_result result{};

   if constexpr (std::is_floating_point_v<T>)
   {
      result = std::from_chars(text.data(), text.data() + text.size(), value);
   }
   else
   {
      result = std::from_chars(text.data(), text.data() + text.size(), value, base);
   }

   if (result.ec != std::errc{} || text.empty())
   {
      return std::nullopt;
   }

   return value;
}

Vector2 rotate(const Vector2& p, float angle)
{
   return Vector2(p.x * std::cos(angle) - p.y * std::sin(angle), p.y * std::cos(angle) + p.x * std::sin(angle));
}
}  // namespace

LensFlare::LensFlare(const Config& config) : _config(config)
{
   _texture = TexturePool::Instance()->getTexture((FLARE_DIRECTORY + config.texture).c_str());
   createBuffers(readGhostStrip(FLARE_DIRECTORY + config.ghosts));
}

LensFlare::~LensFlare()
{
   glDeleteBuffers(1, &_vertex_buffer);
   glDeleteBuffers(1, &_index_buffer);
}

const LensFlare::Config& LensFlare::getConfig() const
{
   return _config;
}

// one ghost per line: x; y; col; row; size; r; g; b[; fields[; height[; angle[; speed]]]]
// (colors in hex), anything that doesn't parse (header, comments) is skipped
std::vector<LensFlare::Ghost> LensFlare::readGhostStrip(const std::string& filename)
{
   std::vector<Ghost> strip;
   std::ifstream file(filename);
   std::string line;

   while (std::getline(file, line))
   {
      std::vector<std::string_view> elements;
      for (const auto element : std::views::split(std::string_view(line), ';'))
      {
         elements.emplace_back(element.begin(), element.end());
      }

      if (elements.size() < 8)
      {
         continue;
      }

      const auto x = parse<float>(elements[0]);
      const auto y = parse<float>(elements[1]);
      const auto column = parse<int32_t>(elements[2]);
      const auto row = parse<int32_t>(elements[3]);
      const auto r = parse<int32_t>(elements[5], 16);
      const auto g = parse<int32_t>(elements[6], 16);
      const auto b = parse<int32_t>(elements[7], 16);

      if (!x || !y || !column || !row || !r || !g || !b)
      {
         continue;
      }

      Ghost ghost;
      ghost.ray_position = *x;
      ghost.offset_y = *y;
      ghost.tile_column = *column;
      ghost.tile_row = *row;
      ghost.size = parse<float>(elements[4]).value_or(0.0f);
      ghost.color = Vector(*r / 255.0f, *g / 255.0f, *b / 255.0f);

      if (elements.size() > 8)
      {
         if (const auto fields = parse<int32_t>(elements[8]))
         {
            ghost.width = *fields;
            ghost.height = *fields;
         }
      }

      if (elements.size() > 9)
      {
         if (const auto height = parse<int32_t>(elements[9]))
         {
            ghost.height = *height;
         }
      }

      if (elements.size() > 10)
      {
         ghost.angle = parse<float>(elements[10]).value_or(ghost.angle);
      }

      if (elements.size() > 11)
      {
         ghost.speed = parse<float>(elements[11]).value_or(ghost.speed);
      }

      strip.push_back(ghost);
   }

   return strip;
}

void LensFlare::createBuffers(const std::vector<Ghost>& ghosts)
{
   std::vector<LensFlareVertex> vertices;
   std::vector<uint16_t> indices;

   for (const auto& ghost : ghosts)
   {
      const float angle = (ghost.angle == 0.0f) ? _config.angle : ghost.angle;

      const float tile_width = 1.0f / TILE_COLUMNS;
      const float tile_height = 1.0f / TILE_ROWS;
      const float u0 = ghost.tile_column * tile_width;
      const float v0 = ghost.tile_row * tile_height;
      const float u1 = u0 + tile_width * ghost.width;
      const float v1 = v0 + tile_height * ghost.height;

      const float half = 0.5f * ghost.size;
      const std::array<Vector2, 4> corners = {
         rotate(Vector2(half, half), angle),
         rotate(Vector2(-half, half), angle),
         rotate(Vector2(-half, -half), angle),
         rotate(Vector2(half, -half), angle),
      };
      const std::array<Vector2, 4> uvs = {Vector2(u1, v1), Vector2(u0, v1), Vector2(u0, v0), Vector2(u1, v0)};

      const auto first = static_cast<uint16_t>(vertices.size());

      for (size_t i = 0; i < corners.size(); i++)
      {
         LensFlareVertex vertex;
         vertex.x = corners[i].x;
         vertex.y = corners[i].y;
         vertex.ray_position = ghost.ray_position;
         vertex.rotation_speed = ghost.speed;
         vertex.offset_y = ghost.offset_y;
         vertex.u = uvs[i].x;
         vertex.v = uvs[i].y;
         vertex.r = ghost.color.x;
         vertex.g = ghost.color.y;
         vertex.b = ghost.color.z;
         vertices.push_back(vertex);
      }

      indices.insert(
         indices.end(),
         {first,
          static_cast<uint16_t>(first + 1),
          static_cast<uint16_t>(first + 3),
          static_cast<uint16_t>(first + 3),
          static_cast<uint16_t>(first + 1),
          static_cast<uint16_t>(first + 2)}
      );
   }

   _index_count = static_cast<int32_t>(indices.size());

   glGenBuffers(1, &_vertex_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(LensFlareVertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glGenBuffers(1, &_index_buffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indices.size(), indices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void LensFlare::draw(const Vector2& sun_2d, int32_t time_param, int32_t sun_param, int32_t length_param)
{
   float length = std::sqrt(sun_2d.x * sun_2d.x + sun_2d.y * sun_2d.y);

   if (_config.inverted)
   {
      length = _config.invert_offset - length;
   }

   length -= _config.lower_limit;
   length /= _config.max_length;

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texture.getTexture());

   activeDevice->setParameter(time_param, GlobalTime::Instance()->getTime());
   activeDevice->setParameter(sun_param, sun_2d);
   activeDevice->setParameter(length_param, length);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(LensFlareVertex), (GLvoid*)0);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LensFlareVertex), (GLvoid*)(sizeof(float) * 4));
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(LensFlareVertex), (GLvoid*)(sizeof(float) * 8));

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glDrawElements(GL_TRIANGLES, _index_count, GL_UNSIGNED_SHORT, nullptr);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
