#include "blockmaterial.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include "animation/motionmixer.h"
#include "framework/framebuffer.h"
#include "gldevice.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "image/image.h"
#include "image/imagepool.h"
#include "image/psd.h"
#include "nodes/camera.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

/*
 expected block neighbouring information (in Mesh render flags)

 top left      1
 top center    2   * top
 top right     4
 mid left      8   * left
 mid center    16            <-- this is the mesh itself (ignore)
 mid right     32  * right
 bot left      64
 bot center    128 * down
 bot right     256

 the main directions make up 2^4=16 possible combinations stored as a 4x4 matrix in the texture
*/

namespace
{
// shared ambient occlusion source layers, loaded on first use
std::unique_ptr<PSD> matrix_layers;
// the light pools' shader and quad (beginLightPools()), the blocks' extent in their own space (all blocks are
// the same mesh)
struct
{
   uint32_t shader = 0;
   uint32_t quad = 0;
   int32_t centre = 0;
   int32_t extent = 0;
   int32_t floor = 0;
   int32_t tint = 0;
   int32_t strength = 0;
} pool;
Vector block_min{1e9f, 1e9f, 1e9f};
Vector block_max{-1e9f, -1e9f, -1e9f};
// what glass blocks show through them, see captureBehind(); only copied while some blocks are glass
uint32_t behind_texture = 0;
bool glass_in_use = false;
}  // namespace

BlockMaterial::BlockMaterial() : Material(MAP_DIFFUSE | MAP_REFLECT)
{
}

void BlockMaterial::setColorMap(const std::string& color_map)
{
   replaceTexture(_color_map, color_map);
}

void BlockMaterial::setVaried(bool varied)
{
   _varied = varied;
}

void BlockMaterial::setGlass(std::vector<Vector> tints)
{
   _glass_tints = std::move(tints);
   glass_in_use = !_glass_tints.empty();
}

int32_t BlockMaterial::glassTint(const Matrix& transform)
{
   const Vector at = transform.translation();
   const auto x = static_cast<uint32_t>(static_cast<int32_t>(std::floor(at.x * 2.0f + 0.5f)));
   const auto y = static_cast<uint32_t>(static_cast<int32_t>(std::floor(at.y * 2.0f + 0.5f)));
   return static_cast<int32_t>(((x * 73856093u) ^ (y * 19349663u)) % 3u);
}

void BlockMaterial::beginLightPools()
{
   if (!pool.shader)
   {
      pool.shader = activeDevice().loadShader("glasspool-vert.glsl", "glasspool-frag.glsl");
      pool.centre = activeDevice().getParameterIndex("centre");
      pool.extent = activeDevice().getParameterIndex("extent");
      pool.floor = activeDevice().getParameterIndex("floorHeight");
      pool.tint = activeDevice().getParameterIndex("tint");
      pool.strength = activeDevice().getParameterIndex("strength");

      // two triangles across the quad
      pool.quad = activeDevice().createVertexBuffer(sizeof(Vector) * 6);
      const std::span<Vector> corners = activeDevice().lockVertexBuffer<Vector>(pool.quad, sizeof(Vector) * 6);
      corners[0] = Vector(-1.0f, -1.0f, 0.0f);
      corners[1] = Vector(1.0f, -1.0f, 0.0f);
      corners[2] = Vector(1.0f, 1.0f, 0.0f);
      corners[3] = Vector(-1.0f, -1.0f, 0.0f);
      corners[4] = Vector(1.0f, 1.0f, 0.0f);
      corners[5] = Vector(-1.0f, 1.0f, 0.0f);
      activeDevice().unlockVertexBuffer(pool.quad);
   }

   activeDevice().setShader(pool.shader);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDepthMask(GL_FALSE);
   glBindBuffer(GL_ARRAY_BUFFER, pool.quad);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector), nullptr);
}

void BlockMaterial::drawLightPool(const Matrix& transform, const Vector& tint, float strength, float reach)
{
   // in the block's own space: round its foot, from its bottom a hair up so it lies on the floor
   const Vector centre = (block_min + block_max) * 0.5f;
   const Vector size = block_max - block_min;
   activeDevice().setParameter(pool.centre, centre);
   activeDevice().setParameter(pool.extent, Vector2(size.x * reach, size.y * reach));
   activeDevice().setParameter(pool.floor, block_min.z + size.z * 0.01f);
   activeDevice().setParameter(pool.tint, tint);
   activeDevice().setParameter(pool.strength, strength);
   activeDevice().push(transform);
   glDrawArrays(GL_TRIANGLES, 0, 6);
   activeDevice().pop();
}

void BlockMaterial::endLightPools()
{
   glDisableVertexAttribArray(0);
   glDepthMask(GL_TRUE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_BLEND);
   activeDevice().setShader(0);
}

void BlockMaterial::drawLightPools()
{
   beginLightPools();

   for (const Buffer& buffer : _buffers)
   {
      const Geometry& geometry = buffer.geometry;
      if (geometry.isVisible())
      {
         drawLightPool(geometry.getTransform(), _glass_tints[glassTint(geometry.getTransform()) % _glass_tints.size()], 0.7f, 1.25f);
      }
   }

   endLightPools();
}

void BlockMaterial::captureBehind()
{
   if (!glass_in_use)
   {
      return;
   }

   if (!behind_texture)
   {
      glGenTextures(1, &behind_texture);
   }

   const auto [x, y, width, height] = activeDevice().getViewPort();
   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, behind_texture);
   FrameBuffer::copyTexImage(x, y, width, height);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glActiveTexture(GL_TEXTURE0);
}

BlockMaterial::BlockMaterial(
   const std::string& color_map,
   const std::string& diffuse_map,
   const std::string& specular_map,
   const std::string& shadow_map,
   Camera& shadow_camera,
   bool ambient
)
    : Material(MAP_DIFFUSE | MAP_REFLECT), _shadow_camera(shadow_camera)
{
   addTexture(_specular_map, specular_map);
   addTexture(_diffuse_map, diffuse_map);
   addTexture(_color_map, color_map);
   addTexture(_shadow_map, shadow_map);

   // create ambient occlusion 4x4 matrix texture
   std::unique_ptr<Image> matrix;

   if (ambient)
   {
      matrix = std::make_unique<Image>(1024, 2048);
      matrix->clear(0xffff0000);

      if (!matrix_layers)
      {
         matrix_layers = std::make_unique<PSD>();
         matrix_layers->load("block-shadow.psd");
      }

      const std::array<std::reference_wrapper<const Image>, 5> sources = {
         matrix_layers->getLayer("back 3")->getImage(),
         matrix_layers->getLayer("left 3")->getImage(),
         matrix_layers->getLayer("right 3")->getImage(),
         matrix_layers->getLayer("front 3")->getImage(),
         matrix_layers->getLayer("none")->getImage()
      };

      const int32_t width = matrix_layers->getWidth();
      const int32_t height = matrix_layers->getHeight();
      for (int32_t y = 0; y < 4; y++)
      {
         for (int32_t x = 0; x < 4; x++)
         {
            const int32_t flags = (y << 2) | x;

            Image image(sources[4].get().getWidth(), sources[4].get().getHeight());
            image.copy(0, 0, sources[4]);
            for (int32_t test = 0; test < 4; test++)
            {
               if (flags & (1 << test))
               {
                  image.minimum(sources[test]);
               }
            }

            // texture is bottom->up
            putImage(*matrix, x * (width + 4), y * (height + 4), image);
         }
      }
   }
   else
   {
      matrix = std::make_unique<Image>(1, 1);
      matrix->clear(0xffffffff);
   }

   addTexture(_ambient_map, std::move(matrix));
}

void BlockMaterial::putScanline(std::span<uint32_t> destination, std::span<const uint32_t> source)
{
   // replicate first and last pixel
   destination[0] = source.front();
   std::ranges::copy(source, destination.begin() + 1);
   destination[source.size() + 1] = source.back();
}

void BlockMaterial::putImage(Image& target, int32_t x_position, int32_t y_position, const Image& source)
{
   const int32_t height = source.getHeight();

   putScanline(target.getScanline(y_position).subspan(x_position), source.getScanline(0));

   for (int32_t y = 0; y < height; y++)
   {
      putScanline(target.getScanline(y + y_position + 1).subspan(x_position), source.getScanline(y));
   }

   putScanline(target.getScanline(y_position + height + 1).subspan(x_position), source.getScanline(height - 1));
}

void BlockMaterial::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0).name());
   addTexture(_specular_map, getTextureSlot(1).name());

   init();
}

void BlockMaterial::init()
{
   _shader = activeDevice().loadShader("blockmaterial-vert.glsl", "blockmaterial-frag.glsl");

   _param_specular = activeDevice().getParameterIndex("specularmap");
   _param_diffuse = activeDevice().getParameterIndex("diffusemap");
   _param_shadow = activeDevice().getParameterIndex("shadowmap");
   _param_texture = activeDevice().getParameterIndex("texturemap");
   _param_ambient = activeDevice().getParameterIndex("ambientmap");

   _param_camera = activeDevice().getParameterIndex("camera");
   _param_shadow_camera = activeDevice().getParameterIndex("shadowCamera");
   _param_offset = activeDevice().getParameterIndex("uvOffset");
   _param_variation = activeDevice().getParameterIndex("variation");
   _param_glass = activeDevice().getParameterIndex("glass");
   _param_tints = {activeDevice().getParameterIndex("tint0"), activeDevice().getParameterIndex("tint1"),
                   activeDevice().getParameterIndex("tint2")};
   _param_viewport = activeDevice().getParameterIndex("viewport");
   _param_box_min = activeDevice().getParameterIndex("boxMin");
   _param_box_max = activeDevice().getParameterIndex("boxMax");
   _param_refraction = activeDevice().getParameterIndex("refraction");
   _param_tint_choice = activeDevice().getParameterIndex("tintChoice");
}

void BlockMaterial::addGeometry(Geometry& geometry)
{
   std::optional<std::reference_wrapper<VertexBuffer>> pooled = _pool->get(geometry);
   if (!pooled)
   {
      VertexBuffer& vertex_buffer = _pool->add(geometry);
      pooled = vertex_buffer;

      const std::span<const Vector> vertices = geometry.getVertices();
      const std::span<const Vector> normals = geometry.getNormals();
      const std::span<const UV> uv = geometry.getUV(1);

      activeDevice().allocateVertexBuffer(vertex_buffer.getVertexBuffer(), sizeof(Vertex) * geometry.getVertexCount());
      const std::span<Vertex> destination = activeDevice().lockVertexBuffer<Vertex>(vertex_buffer.getVertexBuffer());
      for (int32_t i = 0; i < geometry.getVertexCount(); i++)
      {
         destination[i].position.x = vertices[i].x;
         destination[i].position.y = vertices[i].y;
         destination[i].position.z = vertices[i].z;

         destination[i].normal.x = normals[i].x;
         destination[i].normal.y = normals[i].y;
         destination[i].normal.z = normals[i].z;

         destination[i].uv.u = uv[i].u;
         destination[i].uv.v = uv[i].v;

         _box_min = Vector(std::min(_box_min.x, vertices[i].x), std::min(_box_min.y, vertices[i].y), std::min(_box_min.z, vertices[i].z));
         _box_max = Vector(std::max(_box_max.x, vertices[i].x), std::max(_box_max.y, vertices[i].y), std::max(_box_max.z, vertices[i].z));
         block_min = _box_min;
         block_max = _box_max;
      }
      activeDevice().unlockVertexBuffer(vertex_buffer.getVertexBuffer());

      vertex_buffer.setIndexBuffer(geometry.getIndices());
   }

   _buffers.push_back({geometry, *pooled});
}

void BlockMaterial::begin()
{
   Material::begin();

   const bool glass = !_glass_tints.empty() && behind_texture;

   glBindTexture(GL_TEXTURE_2D, _diffuse_map);

   glActiveTexture(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   glActiveTexture(GL_TEXTURE2_ARB);
   glBindTexture(GL_TEXTURE_2D, _shadow_map);

   glActiveTexture(GL_TEXTURE3_ARB);
   glBindTexture(GL_TEXTURE_2D, _ambient_map);

   glActiveTexture(GL_TEXTURE4_ARB);
   glBindTexture(GL_TEXTURE_2D, _specular_map);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_diffuse, 0);
   activeDevice().bindSampler(_param_texture, 1);
   activeDevice().bindSampler(_param_shadow, 2);
   activeDevice().bindSampler(_param_ambient, 3);
   activeDevice().bindSampler(_param_specular, 4);
   activeDevice().setParameter(_param_variation, _varied ? 1.0f : 0.0f);
   activeDevice().setParameter(_param_glass, glass ? 1.0f : 0.0f);
   if (glass)
   {
      glActiveTexture(GL_TEXTURE5);
      glBindTexture(GL_TEXTURE_2D, behind_texture);
      glActiveTexture(GL_TEXTURE0);
      activeDevice().bindSampler(_param_refraction, 5);
      for (size_t i = 0; i < _param_tints.size(); i++)
      {
         activeDevice().setParameter(_param_tints[i], _glass_tints[i % _glass_tints.size()]);
      }
      const auto [x, y, width, height] = activeDevice().getViewPort();
      activeDevice().setParameter(_param_viewport, Vector4(static_cast<float>(x), static_cast<float>(y),
                                                        static_cast<float>(width), static_cast<float>(height)));
      activeDevice().setParameter(_param_box_min, _box_min);
      activeDevice().setParameter(_param_box_max, _box_max);
   }

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);

   Matrix camera;
   if (_shadow_camera)
   {
      auto& device = static_cast<GLDevice&>(activeDevice());
      device.pushProjection();

      camera = _shadow_camera->get().getTransform().getView();
      float fov = _shadow_camera->get().getFOV();
      fov = std::tan(fov * 0.5) * 0.75;
      const float z_near = _shadow_camera->get().getNear();
      const float z_far = _shadow_camera->get().getFar();

      if (const auto reference = getGeometry(0))
      {
         if (const auto scene = reference->get().getParent().getRoot())
         {
            camera = scene->get().getGlobalTransform() * camera;
         }
      }

      activeDevice().setCamera(camera, fov, z_near, z_far, _shadow_camera->get().getPerspectiveMode());

      camera = device.getProjectionMatrix();
      camera.normalizeZ();
      device.popProjection();
   }
   activeDevice().setParameter(_param_shadow_camera, camera);
}

void BlockMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);

   glActiveTexture(GL_TEXTURE0);

   activeDevice().setShader(0);
}

void BlockMaterial::setLightProbes(const std::string& diffuse_map, const std::string& specular_map)
{
   replaceTexture(_diffuse_map, diffuse_map);
   replaceTexture(_specular_map, specular_map);
}

void BlockMaterial::setShadowCookie(const std::string& shadow_map)
{
   replaceTexture(_shadow_map, shadow_map);
}

void BlockMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      const VertexBuffer& vertex_buffer = buffer.vertex_buffer;
      Geometry& geometry = buffer.geometry;

      if (geometry.isVisible())
      {
         const Mesh& mesh = geometry.getParent();

         const int32_t top = mesh.getRenderFlags() >> 1 & 1;
         const int32_t left = mesh.getRenderFlags() >> 3 & 1;
         const int32_t right = mesh.getRenderFlags() >> 5 & 1;
         const int32_t bottom = mesh.getRenderFlags() >> 7 & 1;
         const int32_t flags = (top) | (left << 1) | (right << 2) | (bottom << 3);
         const int32_t x = flags & 3;
         const int32_t y = (flags >> 2) & 3;

         const Matrix inverse_view = (geometry.getTransform() * _camera).invert();
         const Vector object_space_camera = inverse_view.translation();
         activeDevice().setParameter(_param_camera, object_space_camera);
         activeDevice().setParameter(_param_offset, Vector(x, y));
         activeDevice().setParameter(_param_tint_choice, static_cast<float>(glassTint(geometry.getTransform())));

         activeDevice().push(geometry.getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer.getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         activeDevice().pop();
      }
   }

   end();

   if (!_glass_tints.empty() && behind_texture)
   {
      drawLightPools();
   }
}

void BlockMaterial::update(float /*frame*/, const Matrix& camera)
{
   _camera = camera;
}
