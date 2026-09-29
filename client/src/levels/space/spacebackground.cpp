#include "spacebackground.h"

#include "framework/globaltime.h"
#include "gldevice.h"
#include "math/matrix.h"
#include "menus/defaultshader.h"
#include "render/texturepool.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

namespace
{
constexpr int32_t EARTH_SEGMENTS_VERTICAL = 128;
constexpr int32_t EARTH_SEGMENTS_HORIZONTAL = 256;
constexpr int32_t AURA_SEGMENTS = 128;

constexpr float ASPECT_Y = 9.0f / 16.0f;
constexpr float FRUSTUM_SCALE = 0.05f;

constexpr float PI = std::numbers::pi_v<float>;

float radians(float degrees)
{
   return degrees * PI / 180.0f;
}

struct EarthVertex
{
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   float tx = 0.0f;
   float ty = 0.0f;
   float tz = 0.0f;
   float u = 0.0f;
   float v = 0.0f;
};

struct AuraVertex
{
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   float r = 0.0f;
   float g = 0.0f;
   float b = 0.0f;
   float a = 0.0f;
};
}  // namespace

SpaceBackground::SpaceBackground()
{
   _star_field_texture = TexturePool::Instance()->getTexture("starfield");
   _earth_texture = TexturePool::Instance()->getTexture("earth_color");
   _earth_normal_texture = TexturePool::Instance()->getTexture("earth_normal");

   _earth_shader = activeDevice->loadShader("earth-vert.glsl", "earth-frag.glsl");
   _light_param = activeDevice->getParameterIndex("osLightPos");
   _camera_param = activeDevice->getParameterIndex("osCameraPos");
   _cloud_move_param = activeDevice->getParameterIndex("moveCloud");
   _texture_map_param = activeDevice->getParameterIndex("texturemap");
   _normal_map_param = activeDevice->getParameterIndex("normalmap");

   _aura_shader = activeDevice->loadShader("earthaura-vert.glsl", "earthaura-frag.glsl");

   initEarth();
   initAura();

   // full screen quad for the starfield, the uvs scroll in drawStarField()
   glGenBuffers(1, &_quad_vertex_buffer);
}

SpaceBackground::~SpaceBackground()
{
   glDeleteBuffers(1, &_earth_vertex_buffer);
   glDeleteBuffers(1, &_earth_index_buffer);
   glDeleteBuffers(1, &_aura_vertex_buffer);
   glDeleteBuffers(1, &_quad_vertex_buffer);
}

void SpaceBackground::initEarth()
{
   std::vector<EarthVertex> vertices;
   vertices.reserve(static_cast<size_t>((EARTH_SEGMENTS_VERTICAL + 1) * (EARTH_SEGMENTS_HORIZONTAL + 1)));

   for (int32_t sy = 0; sy <= EARTH_SEGMENTS_VERTICAL; sy++)
   {
      const float v = static_cast<float>(sy) / EARTH_SEGMENTS_VERTICAL;
      const float y = std::cos(v * PI);

      // avoid the singularity at the poles
      float r = std::sin(v * PI);
      if (std::fabs(r) <= 0.0001f)
      {
         r = 0.0001f;
      }

      for (int32_t sx = 0; sx <= EARTH_SEGMENTS_HORIZONTAL; sx++)
      {
         const float u = static_cast<float>(sx) / EARTH_SEGMENTS_HORIZONTAL;
         const float x = std::cos(u * 2.0f * PI) * r;
         const float z = std::sin(u * 2.0f * PI) * r;
         const Vector tangent = Vector::normalize(Vector(-z, 0.0f, x));

         vertices.push_back({x, y, z, tangent.x, tangent.y, tangent.z, u, v});
      }
   }

   std::vector<uint16_t> indices;
   indices.reserve(static_cast<size_t>(EARTH_SEGMENTS_HORIZONTAL * EARTH_SEGMENTS_VERTICAL * 6));

   for (int32_t sy = 0; sy < EARTH_SEGMENTS_VERTICAL; sy++)
   {
      const int32_t r1 = sy * (EARTH_SEGMENTS_HORIZONTAL + 1);
      const int32_t r2 = (sy + 1) * (EARTH_SEGMENTS_HORIZONTAL + 1);

      for (int32_t sx = 0; sx < EARTH_SEGMENTS_HORIZONTAL; sx++)
      {
         for (const auto index : {r1 + sx, r1 + sx + 1, r2 + sx, r1 + sx + 1, r2 + sx + 1, r2 + sx})
         {
            indices.push_back(static_cast<uint16_t>(index));
         }
      }
   }

   _earth_index_count = static_cast<int32_t>(indices.size());

   glGenBuffers(1, &_earth_vertex_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _earth_vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(EarthVertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glGenBuffers(1, &_earth_index_buffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _earth_index_buffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indices.size(), indices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// a thin ring fading from the aura color (inside) to transparent (outside)
void SpaceBackground::initAura()
{
   constexpr float z = 0.5f;
   const float r1 = std::sqrt(1.0f - z * z);
   const float r2 = r1 + 0.05f;
   const Vector color(0.7f, 0.9f, 1.5f);

   std::vector<AuraVertex> vertices;
   float a = 0.0f;
   const float da = PI * 2.0f / AURA_SEGMENTS;

   for (int32_t i = 0; i <= AURA_SEGMENTS; i++)
   {
      const float x = std::sin(a);
      const float y = std::cos(a);
      a += da;

      vertices.push_back({x * r2, y * r2, z, 0.0f, 0.0f, 0.0f, 0.0f});
      vertices.push_back({x * r1, y * r1, z, color.x, color.y, color.z, 1.0f});
   }

   _aura_vertex_count = static_cast<int32_t>(vertices.size());

   glGenBuffers(1, &_aura_vertex_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _aura_vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(AuraVertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SpaceBackground::animate(float dt)
{
   _earth_rotation += dt * 0.01f;
}

void SpaceBackground::draw()
{
   auto* device = static_cast<GLDevice*>(activeDevice);
   device->clear();

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   device->pushProjection();

   drawStarField();

   // camera in front of the earth; the view translation lives in the projection like the scene camera
   device->setProjectionMatrix(
      Matrix::position(-_camera.x, -_camera.y, -_camera.z) *
      Matrix::frustum(-FRUSTUM_SCALE, FRUSTUM_SCALE, -ASPECT_Y * FRUSTUM_SCALE, ASPECT_Y * FRUSTUM_SCALE, 0.1f, 10.0f)
   );

   drawEarth();
   drawAura();

   device->popProjection();

   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
}

void SpaceBackground::drawStarField()
{
   auto* device = static_cast<GLDevice*>(activeDevice);
   device->setProjectionMatrix(Matrix());

   const float offset = GlobalTime::Instance()->getTime() * 0.002f;
   const std::array<float, 30> vertices = {
      -1.0f, -1.0f, 0.0f, offset + 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, offset + 2.0f, 0.0f, 1.0f,  1.0f, 0.0f, offset + 2.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, offset + 0.0f, 0.0f, 1.0f, 1.0f,  0.0f, offset + 2.0f, 1.0f, -1.0f, 1.0f, 0.0f, offset + 0.0f, 1.0f,
   };

   glDisable(GL_BLEND);

   device->setShader(getDefaultMenuShader());
   device->setParameter(getDefaultMenuShaderAlphaParam(), 1.0f);
   device->push(Matrix());

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _star_field_texture.getTexture());

   glBindBuffer(GL_ARRAY_BUFFER, _quad_vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<const void*>(sizeof(float) * 3));
   glDrawArrays(GL_TRIANGLES, 0, 6);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   device->pop();
   device->setShader(0);
}

void SpaceBackground::drawEarth()
{
   // spin around y, then tilt by 20 degrees around x (Matrix::rotateX turns the opposite way to
   // glRotatef, rotateY and rotateZ match it)
   const Matrix model = Matrix::rotateX(radians(-20.0f)) * Matrix::rotateY(radians(_earth_rotation));
   const Matrix inverse_model = model.invert();

   activeDevice->setShader(_earth_shader);
   activeDevice->push(model);

   activeDevice->setParameter(_light_param, inverse_model * Vector(20.0f, -20.0f, -1.0f));
   activeDevice->setParameter(_camera_param, inverse_model * _camera);
   activeDevice->setParameter(_cloud_move_param, Vector(GlobalTime::Instance()->getTime() * 0.001f, 0.0f, 0.0f));

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _earth_texture.getTexture());
   activeDevice->bindSampler(_texture_map_param, 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _earth_normal_texture.getTexture());
   activeDevice->bindSampler(_normal_map_param, 1);

   glBindBuffer(GL_ARRAY_BUFFER, _earth_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(EarthVertex), nullptr);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(EarthVertex), reinterpret_cast<const void*>(sizeof(float) * 3));
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(EarthVertex), reinterpret_cast<const void*>(sizeof(float) * 6));

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _earth_index_buffer);
   glDrawElements(GL_TRIANGLES, _earth_index_count, GL_UNSIGNED_SHORT, nullptr);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);

   activeDevice->pop();
   activeDevice->setShader(0);
}

void SpaceBackground::drawAura()
{
   // the ring faces the camera, slightly tilted to match the earth's silhouette
   const Matrix model = Matrix::rotateY(radians(-1.0f)) * Matrix::rotateX(radians(12.0f));

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(_aura_shader);
   activeDevice->push(model);

   glBindBuffer(GL_ARRAY_BUFFER, _aura_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AuraVertex), nullptr);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(AuraVertex), reinterpret_cast<const void*>(sizeof(float) * 3));
   glDrawArrays(GL_TRIANGLE_STRIP, 0, _aura_vertex_count);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   activeDevice->pop();
   activeDevice->setShader(0);

   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
