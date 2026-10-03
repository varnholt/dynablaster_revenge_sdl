#include "blockmaterial.h"
#include <array>
#include <cmath>
#include <memory>
#include "animation/motionmixer.h"
#include "gldevice.h"
#include "image/image.h"
#include "image/imagepool.h"
#include "image/psd.h"
#include "nodes/camera.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "render/renderbuffer.h"
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
}  // namespace

BlockMaterial::BlockMaterial(SceneGraph* scene) : Material(scene, MAP_DIFFUSE | MAP_REFLECT)
{
}

BlockMaterial::BlockMaterial(
   SceneGraph* scene,
   const char* color_map,
   const char* diffuse_map,
   const char* specular_map,
   const char* shadow_map,
   Camera* shadow_camera,
   bool ambient
)
    : Material(scene, MAP_DIFFUSE | MAP_REFLECT), _shadow_camera(shadow_camera)
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

      const std::array<const Image*, 5> sources = {
         &matrix_layers->getLayer("back 3")->getImage(),
         &matrix_layers->getLayer("left 3")->getImage(),
         &matrix_layers->getLayer("right 3")->getImage(),
         &matrix_layers->getLayer("front 3")->getImage(),
         &matrix_layers->getLayer("none")->getImage()
      };

      const int32_t width = matrix_layers->getWidth();
      const int32_t height = matrix_layers->getHeight();
      for (int32_t y = 0; y < 4; y++)
      {
         for (int32_t x = 0; x < 4; x++)
         {
            const int32_t flags = (y << 2) | x;

            Image image(sources[4]->getWidth(), sources[4]->getHeight());
            image.copy(0, 0, *sources[4]);
            for (int32_t test = 0; test < 4; test++)
            {
               if (flags & (1 << test))
               {
                  image.minimum(*sources[test]);
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

void BlockMaterial::putScanline(uint32_t* destination, const uint32_t* source, int32_t width)
{
   // replicate first and last pixel
   destination[0] = source[0];
   for (int32_t x = 0; x < width; x++)
   {
      destination[x + 1] = source[x];
   }
   destination[width + 1] = source[width - 1];
}

void BlockMaterial::putImage(Image& target, int32_t x_position, int32_t y_position, const Image& source)
{
   const int32_t width = source.getWidth();
   const int32_t height = source.getHeight();

   putScanline(target.getScanline(y_position) + x_position, source.getScanline(0), width);

   for (int32_t y = 0; y < height; y++)
   {
      putScanline(target.getScanline(y + y_position + 1) + x_position, source.getScanline(y), width);
   }

   putScanline(target.getScanline(y_position + height + 1) + x_position, source.getScanline(height - 1), width);
}

void BlockMaterial::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0)->name());
   addTexture(_specular_map, getTextureSlot(1)->name());

   init();
}

void BlockMaterial::init()
{
   _shader = activeDevice->loadShader("blockmaterial-vert.glsl", "blockmaterial-frag.glsl");

   _param_specular = activeDevice->getParameterIndex("specularmap");
   _param_diffuse = activeDevice->getParameterIndex("diffusemap");
   _param_shadow = activeDevice->getParameterIndex("shadowmap");
   _param_texture = activeDevice->getParameterIndex("texturemap");
   _param_ambient = activeDevice->getParameterIndex("ambientmap");

   _param_camera = activeDevice->getParameterIndex("camera");
   _param_shadow_camera = activeDevice->getParameterIndex("shadowCamera");
   _param_offset = activeDevice->getParameterIndex("uvOffset");
}

void BlockMaterial::addGeometry(Geometry* geometry)
{
   VertexBuffer* vertex_buffer = _pool->get(geometry);
   if (!vertex_buffer)
   {
      vertex_buffer = _pool->add(geometry);

      const Vector* vertices = geometry->getVertices();
      const Vector* normals = geometry->getNormals();
      const UV* uv = geometry->getUV(1);

      activeDevice->allocateVertexBuffer(vertex_buffer->getVertexBuffer(), sizeof(Vertex) * geometry->getVertexCount());
      volatile Vertex* destination = static_cast<Vertex*>(activeDevice->lockVertexBuffer(vertex_buffer->getVertexBuffer()));
      for (int32_t i = 0; i < geometry->getVertexCount(); i++)
      {
         destination[i].position.x = vertices[i].x;
         destination[i].position.y = vertices[i].y;
         destination[i].position.z = vertices[i].z;

         destination[i].normal.x = normals[i].x;
         destination[i].normal.y = normals[i].y;
         destination[i].normal.z = normals[i].z;

         destination[i].uv.u = uv[i].u;
         destination[i].uv.v = uv[i].v;
      }
      activeDevice->unlockVertexBuffer(vertex_buffer->getVertexBuffer());

      vertex_buffer->setIndexBuffer(geometry->getIndices(), geometry->getIndexCount());
   }

   _buffers.push_back({geometry, vertex_buffer});
}

void BlockMaterial::begin()
{
   Material::begin();

   glBindTexture(GL_TEXTURE_2D, _diffuse_map);

   glActiveTexture(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   glActiveTexture(GL_TEXTURE2_ARB);
   glBindTexture(GL_TEXTURE_2D, _shadow_map);

   glActiveTexture(GL_TEXTURE3_ARB);
   glBindTexture(GL_TEXTURE_2D, _ambient_map);

   glActiveTexture(GL_TEXTURE4_ARB);
   glBindTexture(GL_TEXTURE_2D, _specular_map);

   activeDevice->setShader(_shader);
   activeDevice->bindSampler(_param_diffuse, 0);
   activeDevice->bindSampler(_param_texture, 1);
   activeDevice->bindSampler(_param_shadow, 2);
   activeDevice->bindSampler(_param_ambient, 3);
   activeDevice->bindSampler(_param_specular, 4);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);

   Matrix camera;
   if (_shadow_camera)
   {
      GLDevice* device = static_cast<GLDevice*>(activeDevice);
      device->pushProjection();

      camera = _shadow_camera->getTransform().getView();
      float fov = _shadow_camera->getFOV();
      fov = std::tan(fov * 0.5) * 0.75;
      const float z_near = _shadow_camera->getNear();
      const float z_far = _shadow_camera->getFar();

      Geometry* reference = getGeometry(0);
      if (reference)
      {
         Node* parent = reference->getParent();
         if (parent)
         {
            SceneGraph* scene = parent->getRoot();
            if (scene)
            {
               camera = scene->getGlobalTransform() * camera;
            }
         }
      }

      activeDevice->setCamera(camera, fov, z_near, z_far, _shadow_camera->getPerspectiveMode());

      camera = device->getProjectionMatrix();
      camera.normalizeZ();
      device->popProjection();
   }
   activeDevice->setParameter(_param_shadow_camera, camera);
}

void BlockMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);

   glActiveTexture(GL_TEXTURE0);

   activeDevice->setShader(0);
}

void BlockMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      VertexBuffer* vertex_buffer = buffer.vertex_buffer;
      Geometry* geometry = buffer.geometry;

      if (geometry->isVisible())
      {
         const Mesh* mesh = static_cast<const Mesh*>(geometry->getParent());

         const int32_t top = mesh->getRenderFlags() >> 1 & 1;
         const int32_t left = mesh->getRenderFlags() >> 3 & 1;
         const int32_t right = mesh->getRenderFlags() >> 5 & 1;
         const int32_t bottom = mesh->getRenderFlags() >> 7 & 1;
         const int32_t flags = (top) | (left << 1) | (right << 2) | (bottom << 3);
         const int32_t x = flags & 3;
         const int32_t y = (flags >> 2) & 3;

         const Matrix inverse_view = (geometry->getTransform() * _camera).invert();
         const Vector object_space_camera = inverse_view.translation();
         activeDevice->setParameter(_param_camera, object_space_camera);
         activeDevice->setParameter(_param_offset, Vector(x, y));

         activeDevice->push(geometry->getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer->getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer->getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         activeDevice->pop();
      }
   }

   end();
}

void BlockMaterial::update(float /*frame*/, Node** /*node_list*/, const Matrix& camera)
{
   _camera = camera;
}
