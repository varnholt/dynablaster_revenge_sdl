// reference implementation of a dummy material

#include "material.h"
#include <array>
#include <cstdio>
#include <format>
#include <string>
#include "gldevice.h"
#include "image/image.h"
#include "image/imagepool.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "render/geometry.h"
#include "render/renderbuffer.h"
#include "render/texture.h"
#include "render/texturepool.h"
#include "render/vertexbuffer.h"
#include "renderdevice.h"
#include "textureslot.h"
#include "tools/stream.h"

namespace
{
int32_t dummy_counter = 1;

std::string toStdString(const String& text)
{
   return text.isEmpty() ? std::string() : std::string(text.data());
}

void writeText(Stream* stream, std::string text)
{
   stream->writeData(text.data(), static_cast<int32_t>(text.size()));
}
}  // namespace

Material::Material(SceneGraph* scene, int32_t id) : _id(id), _pool(std::make_unique<VertexBufferPool>())
{
   if (scene)
   {
      scene->addMaterial(this);
   }
}

Material::~Material() = default;

void Material::getBoundingRect(Vector& min, Vector& max, const Matrix&)
{
   max = min = Vector(0.0f, 0.0f, 0.0f);
}

Vector Material::getCenter2d(const Matrix&) const
{
   return Vector(0.0f);
}

const std::vector<Material::Buffer>& Material::getBuffers() const
{
   return _buffers;
}

//! get geometry by given index
Geometry* Material::getGeometry(int32_t index) const
{
   if (index >= 0 && index < size())
   {
      return _buffers[index].geometry;
   }
   return nullptr;
}

int32_t Material::getDebug() const
{
   return _debug;
}

void Material::setDebug(int32_t value)
{
   _debug = value;
}

int32_t Material::geometryCount() const
{
   return size();
}

void Material::add(Geometry* geometry)
{
   _geometry_queue.push_back(geometry);
}

int32_t Material::size() const
{
   return static_cast<int32_t>(_buffers.size());
}

void Material::addTexture(Texture& texture, const char* filename, int32_t flags)
{
   addTexture(texture, std::make_unique<Image>(filename), flags);
}

void Material::addTexture(Texture& texture, std::unique_ptr<Image> image, int32_t flags)
{
   _texture_queue.push_back({std::move(image), flags, &texture});
}

void Material::prepare()
{
   // both queues are processed last-in first-out
   while (!_geometry_queue.empty())
   {
      Geometry* geometry = _geometry_queue.back();
      _geometry_queue.pop_back();
      addGeometry(geometry);
   }

   while (!_texture_queue.empty())
   {
      PendingTexture pending = std::move(_texture_queue.back());
      _texture_queue.pop_back();
      *pending.texture = TexturePool::Instance()->getTexture(pending.image.get(), pending.flags);
   }
}

void Material::begin()
{
   if (!_initialized)
   {
      init();
      _initialized = true;
   }

   prepare();
}

uint32_t Material::uploadMap(const Image& image, int32_t flags)
{
   return activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight(), flags);
}

void Material::updateMap(uint32_t texture, const Image& image, int32_t flags)
{
   glBindTexture(GL_TEXTURE_2D, texture);

   return activeDevice->updateTexture(image.getData(), image.getWidth(), image.getHeight(), flags);
}

uint32_t Material::uploadCubeMap(const Image& image)
{
   const int32_t width = image.getWidth();
   const int32_t height = image.getHeight();

   if (width / 3 != height / 4)
   {
      std::printf("expected cubemap as vertical cross!\n");
      return 0;
   }

   GLuint texture = 0;
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   const int32_t size = height >> 2;

   constexpr std::array<GLenum, 6> targets = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
   };

   // position of each face inside the vertical cross (high nibble: row, low nibble: column)
   constexpr std::array<uint8_t, 6> positions = {0x12, 0x10, 0x01, 0x21, 0x11, 0x31};

   for (size_t side = 0; side < targets.size(); side++)
   {
      const int32_t y_position = (positions[side] >> 4 & 3) * size;
      const int32_t x_position = (positions[side] & 3) * size;

      Image face(size, size);

      for (int32_t i = 0; i < size; i++)
      {
         const uint32_t* source = image.getScanline(y_position + i) + x_position;
         if (side == 5)  // back-side must be flipped
         {
            uint32_t* destination = face.getScanline(size - i - 1);
            for (int32_t j = 0; j < size; j++)
            {
               destination[j] = source[size - 1 - j];
            }
         }
         else
         {
            uint32_t* destination = face.getScanline(i);
            for (int32_t j = 0; j < size; j++)
            {
               destination[j] = source[j];
            }
         }
      }

      int32_t level = 0;
      do
      {
         glTexImage2D(targets[side], level, GL_RGBA, face.getWidth(), face.getHeight(), 0, GL_BGRA, GL_UNSIGNED_BYTE, face.getData());
         face = face.downsample();
         level++;
      } while (face.getWidth() > 0 && face.getHeight() > 0);
   }

   return static_cast<uint32_t>(texture);
}

bool Material::getCulling()
{
   return (_flags & 1) == 0;
}

void Material::load(Stream* stream)
{
   const int32_t buffers = stream->getInt();
   _buffers.clear();
   _buffers.reserve(buffers);
   _ambient.load(stream);
   _diffuse.load(stream);
   _specular.load(stream);
   _shininess = stream->getFloat();
   _strength = stream->getFloat();
   _self_illumination = stream->getFloat();
   _ior = stream->getFloat();
   _opacity = stream->getFloat();
   _flags = stream->getInt();

   const int32_t map_count = stream->getInt();
   _slots.clear();
   _slots.reserve(map_count);
   for (int32_t i = 0; i < map_count; i++)
   {
      _slots.push_back(std::make_unique<TextureSlot>(stream));
      stream->skip(72);
   }
}

void Material::write(Stream* stream)
{
   Chunk chunk(stream, _id, name());

   chunk.writeInt(size());
   _ambient.write(&chunk);
   _diffuse.write(&chunk);
   _specular.write(&chunk);
   chunk.writeFloat(_shininess);
   chunk.writeFloat(_strength);
   chunk.writeFloat(_self_illumination);
   chunk.writeFloat(_ior);
   chunk.writeFloat(_opacity);
   chunk.writeInt(_flags);

   chunk.writeInt(static_cast<int32_t>(_slots.size()));
   for (const auto& slot : _slots)
   {
      slot->write(&chunk);
   }
}

TextureSlot* Material::getTextureSlot(int32_t index) const
{
   if (index >= 0 && index < static_cast<int32_t>(_slots.size()))
   {
      return _slots[index].get();
   }
   return nullptr;
}

void Material::addMesh(Mesh* mesh)
{
   if (mesh && mesh->id() == Node::idMesh)
   {
      for (int32_t i = 0; i < mesh->getPartCount(); i++)
      {
         add(mesh->getPart(i));
      }
   }
}

void Material::clear()
{
   _geometry_queue.clear();
   _buffers.clear();
}

void Material::removeMesh(Mesh* mesh)
{
   for (int32_t i = 0; i < mesh->getPartCount(); i++)
   {
      std::erase(_geometry_queue, mesh->getPart(i));
   }

   std::erase_if(_buffers, [mesh](const Buffer& buffer) { return buffer.geometry->getParent() == mesh; });
}

void Material::update(float /*frame*/, Node** node_list, const Matrix&)
{
   for (const Buffer& buffer : _buffers)
   {
      buffer.vertex_buffer->update(node_list);
   }
}

void Material::renderDiffuse()
{
}

void Material::exportGeo(
   Stream* stream,
   const String& name,
   const Matrix& transform,
   Vector* vertices,
   Vector* normals,
   UV* texcoords,
   int32_t vertex_count,
   uint16_t* indices,
   int32_t index_count,
   int32_t index_offset
)
{
   // write object info comment
   if (!name.isEmpty())
   {
      writeText(stream, std::format("# object: {}\n", toStdString(name)));
   }
   else
   {
      writeText(stream, std::format("# object: dummy-{}\n", dummy_counter++));
   }

   writeText(stream, std::format("# vertices: {}\n", vertex_count));
   writeText(stream, std::format("# triangles: {}\n", index_count / 3));

   // write vertices
   stream->writeChar('\n');
   for (int32_t i = 0; i < vertex_count; i++)
   {
      const Vector v = transform * vertices[i];
      writeText(stream, std::format("v {:.9f} {:.9f} {:.9f}\n", -v.x, v.z, v.y));  // flip y/z !
   }

   // write normals
   stream->writeChar('\n');
   for (int32_t i = 0; i < vertex_count; i++)
   {
      const Vector& n = normals[i];
      writeText(stream, std::format("vn {:.6f} {:.6f} {:.6f}\n", -n.x, n.z, n.y));
   }

   // write uv channel
   stream->writeChar('\n');
   for (int32_t i = 0; i < vertex_count; i++)
   {
      if (texcoords)
      {
         writeText(stream, std::format("vt {:.6f} {:.6f} 0.0\n", texcoords[i].u, 1.0f - texcoords[i].v));
      }
      else
      {
         writeText(stream, "vt 0.0 0.0 0.0\n");
      }
   }

   // write triangles - indices start with 1 (not 0)
   stream->writeChar('\n');
   writeText(stream, std::format("g {} \n", toStdString(name)));
   writeText(stream, "s off \n");
   for (int32_t i = 0; i < index_count; i += 3)
   {
      const int32_t i1 = indices[i];
      const int32_t i2 = indices[i + 1];
      const int32_t i3 = indices[i + 2];
      const bool valid = i1 >= 0 && i1 < vertex_count && i2 >= 0 && i2 < vertex_count && i3 >= 0 && i3 < vertex_count;

      if (valid)
      {
         const int32_t f1 = i1 + index_offset;
         const int32_t f2 = i2 + index_offset;
         const int32_t f3 = i3 + index_offset;
         writeText(stream, std::format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", f1, f2, f3));
      }
   }
   stream->writeChar('\n');
   stream->writeChar('\n');
}

void Material::exportOBJ(Stream* stream, int32_t& index_offset)
{
   for (const Buffer& buffer : _buffers)
   {
      Geometry* geometry = buffer.geometry;

      if (!geometry->isVisible())
      {
         continue;
      }

      exportGeo(
         stream,
         geometry->getParent()->name(),
         geometry->getTransform(),
         geometry->getVertices(),
         geometry->getNormals(),
         geometry->getUV(1),
         geometry->getVertexCount(),
         geometry->getIndices(),
         geometry->getIndexCount(),
         index_offset
      );

      index_offset += geometry->getVertexCount();
   }
}
