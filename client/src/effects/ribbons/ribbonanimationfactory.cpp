#include "ribbonanimationfactory.h"

#include "gldevice.h"
#include "math/matrix.h"
#include "math/vector4.h"
#include "render/texturepool.h"

#include <algorithm>
#include <numbers>

namespace
{
constexpr float DURATION = 8.0f;

// ribbon time runs at dt * 0.001 * 13.5 per game tick
constexpr float TIME_SCALE = 0.001f * 13.5f;

constexpr int32_t RIBBONS_PER_ANIMATION = 10;

constexpr int32_t GRID_WIDTH_SEGMENTS = 500;
constexpr int32_t GRID_HEIGHT_SEGMENTS = 2;
constexpr float GRID_WIDTH = 6.0f * std::numbers::pi_v<float>;
constexpr float GRID_HEIGHT = 1.0f;

struct RibbonVertex
{
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   float u = 0.0f;
   float v = 0.0f;
};
}  // namespace

RibbonAnimationFactory::RibbonAnimationFactory()
{
   _texture = TexturePool::Instance()->getTexture("data/effects/ribbons/images/ribbon");

   _shader = activeDevice->loadShader("ribbon-vert.glsl", "ribbon-frag.glsl");
   _time_param = activeDevice->getParameterIndex("time");
   _field_position_param = activeDevice->getParameterIndex("fieldPosition");
   _circle_offset_param = activeDevice->getParameterIndex("circleOffset");
   _texture_param = activeDevice->getParameterIndex("texturemap");

   initBuffers();
}

RibbonAnimationFactory::~RibbonAnimationFactory()
{
   glDeleteBuffers(1, &_vertex_buffer);
   glDeleteBuffers(1, &_index_buffer);
}

void RibbonAnimationFactory::initBuffers()
{
   std::vector<RibbonVertex> vertices(static_cast<size_t>(GRID_WIDTH_SEGMENTS * GRID_HEIGHT_SEGMENTS));

   for (int32_t x = 0; x < GRID_WIDTH_SEGMENTS; x++)
   {
      for (int32_t y = 0; y < GRID_HEIGHT_SEGMENTS; y++)
      {
         auto& vertex = vertices[static_cast<size_t>(y * GRID_WIDTH_SEGMENTS + x)];
         vertex.x = (static_cast<float>(x) / GRID_WIDTH_SEGMENTS) * GRID_WIDTH;
         vertex.y = (static_cast<float>(y) / GRID_HEIGHT_SEGMENTS) * GRID_HEIGHT;
         vertex.u = static_cast<float>(x) / (GRID_WIDTH_SEGMENTS - 1);
         vertex.v = static_cast<float>(y) / (GRID_HEIGHT_SEGMENTS - 1);
      }
   }

   // two triangles per grid cell: (a, b, c) and (c, b, d)
   std::vector<uint16_t> indices;
   for (int32_t x = 0; x < GRID_WIDTH_SEGMENTS - 1; x++)
   {
      for (int32_t y = 0; y < GRID_HEIGHT_SEGMENTS - 1; y++)
      {
         const auto a = static_cast<uint16_t>(y * GRID_WIDTH_SEGMENTS + x);
         const auto b = static_cast<uint16_t>((y + 1) * GRID_WIDTH_SEGMENTS + x);
         const auto c = static_cast<uint16_t>(y * GRID_WIDTH_SEGMENTS + x + 1);
         const auto d = static_cast<uint16_t>((y + 1) * GRID_WIDTH_SEGMENTS + x + 1);
         indices.insert(indices.end(), {a, b, c, c, b, d});
      }
   }

   _index_count = static_cast<int32_t>(indices.size());

   glGenBuffers(1, &_vertex_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(RibbonVertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glGenBuffers(1, &_index_buffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indices.size(), indices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RibbonAnimationFactory::add(float x, float y)
{
   _ribbons.push_back({x + 0.5f, -(y + 0.5f), 0.0f});
}

void RibbonAnimationFactory::clear()
{
   _ribbons.clear();
}

void RibbonAnimationFactory::update(float dt)
{
   if (_ribbons.empty() || dt <= 0.0f)
   {
      return;
   }

   activeDevice->setCulling(false);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(_shader);

   // world space ribbons, the camera lives in the projection matrix
   activeDevice->push(Matrix());

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texture.getTexture());
   activeDevice->bindSampler(_texture_param, 0);

   for (auto& ribbon : _ribbons)
   {
      ribbon.time += dt * TIME_SCALE;
      draw(ribbon);
   }

   std::erase_if(_ribbons, [](const Ribbon& ribbon) { return ribbon.time > DURATION; });

   activeDevice->pop();
   activeDevice->setShader(0);

   activeDevice->setCulling(true);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
}

void RibbonAnimationFactory::draw(const Ribbon& ribbon)
{
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RibbonVertex), nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(RibbonVertex), reinterpret_cast<const void*>(sizeof(float) * 3));
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);

   activeDevice->setParameter(_time_param, ribbon.time);
   activeDevice->setParameter(_field_position_param, Vector4(ribbon.x, ribbon.y, 0.0f, 0.0f));

   float offset = 0.0f;
   for (int32_t i = 0; i < RIBBONS_PER_ANIMATION; i++)
   {
      offset += 2.0f * std::numbers::pi_v<float> / RIBBONS_PER_ANIMATION;
      activeDevice->setParameter(_circle_offset_param, offset);
      glDrawElements(GL_TRIANGLES, _index_count, GL_UNSIGNED_SHORT, nullptr);
   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
