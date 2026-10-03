#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "math/vector.h"
#include "render/texture.h"
#include "render/vertexbufferpool.h"
#include "tools/objectname.h"

class Stream;
class Node;
class Mesh;
class Geometry;
class TextureSlot;
class Image;
class SceneGraph;
class Matrix;
class UV;

// material ids (map-type bit flags) as written by the exporter
constexpr int32_t MAP_AMBIENT = 1;
constexpr int32_t MAP_DIFFUSE = 2;
constexpr int32_t MAP_SPCOLOR = 4;
constexpr int32_t MAP_SPLEVEL = 8;
constexpr int32_t MAP_GLOSS = 16;
constexpr int32_t MAP_SELFILLUM = 32;
constexpr int32_t MAP_OPACITY = 64;
constexpr int32_t MAP_FILTER = 128;
constexpr int32_t MAP_BUMP = 256;
constexpr int32_t MAP_REFLECT = 512;
constexpr int32_t MAP_REFRACT = 1024;
constexpr int32_t MAP_DISPLACE = 2048;

// materials are owned by the scene graph they are added to (SceneGraph::addMaterial())
class Material : public ObjectName
{
public:
   // the geometry is owned by its mesh, the vertex buffer by the material's pool
   struct Buffer
   {
      std::reference_wrapper<Geometry> geometry;
      std::reference_wrapper<VertexBuffer> vertex_buffer;
   };

   explicit Material(int32_t id);
   ~Material() override;

   virtual void init() = 0;
   virtual void begin();
   virtual void end() = 0;
   virtual void exportOBJ(Stream& stream, int32_t& index_offset);
   virtual void exportGeo(
      Stream& stream,
      const std::string& name,
      const Matrix& transform,
      std::span<const Vector> vertices,
      std::span<const Vector> normals,
      std::span<const UV> texcoords,
      std::span<const uint16_t> indices,
      int32_t index_offset
   );
   virtual void addGeometry(Geometry& geometry) = 0;
   std::optional<std::reference_wrapper<Geometry>> getGeometry(int32_t index) const;

   virtual void load(Stream& stream);
   virtual void write(Stream& stream);

   virtual void getBoundingRect(Vector& min, Vector& max, const Matrix& projection);
   virtual Vector getCenter2d(const Matrix& projection) const;

   void prepare();

   void add(Geometry& geometry);
   int32_t size() const;
   void clear();

   bool getCulling();
   int32_t geometryCount() const;
   const std::vector<Buffer>& getBuffers() const;

   // takes ownership of the image, it is deleted once the texture is uploaded
   void addTexture(Texture& texture, std::unique_ptr<Image> image, int32_t flags = 1 | 2 | 4);
   void addTexture(Texture& texture, const std::string& filename, int32_t flags = 1 | 2 | 4);

   virtual void update(float frame, const Matrix& camera);
   virtual void renderDiffuse();
   virtual void addMesh(Mesh& mesh);
   virtual void removeMesh(const Mesh& mesh);

   const TextureSlot& getTextureSlot(int32_t index) const;
   int32_t getTextureSlotCount() const;

   int32_t getDebug() const;
   void setDebug(int32_t value);

   static uint32_t uploadCubeMap(const Image& image);
   static uint32_t uploadMap(const Image& image, int32_t flags = 1);
   static void updateMap(uint32_t texture, const Image& image, int32_t flags = 1);

protected:
   struct PendingTexture
   {
      std::unique_ptr<Image> image;
      int32_t flags = 0;
      std::reference_wrapper<Texture> texture;
   };

   int32_t _id;
   bool _initialized = false;
   std::unique_ptr<VertexBufferPool> _pool;
   std::vector<Buffer> _buffers;
   Vector _ambient;
   Vector _diffuse;
   Vector _specular;
   float _shininess = 0.0f;
   float _opacity = 0.0f;
   float _self_illumination = 0.0f;
   float _strength = 0.0f;
   float _ior = 0.0f;
   int32_t _flags = 0;
   std::vector<std::unique_ptr<TextureSlot>> _slots;
   std::vector<std::reference_wrapper<Geometry>> _geometry_queue;
   std::vector<PendingTexture> _texture_queue;
   int32_t _debug = 0;
};
