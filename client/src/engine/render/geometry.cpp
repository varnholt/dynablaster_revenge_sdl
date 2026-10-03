#include "geometry.h"
#include "animation/motionmixer.h"
#include "math/vector.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "nodes/scenegraph.h"
#include "uv.h"

#include <algorithm>
#include <array>
#include <span>

namespace
{
int32_t _current_geometry_id = 0;
const Matrix _identity;
}  // namespace

Geometry::Geometry(Node* parent) : _id(_current_geometry_id++), _parent(parent)
{
}

void Geometry::copy(const Geometry& geometry)
{
   const Data& source = *geometry._data;
   auto data = std::make_shared<Data>();
   data->indices = source.indices;
   data->vertices = source.vertices;
   data->colors = source.colors;
   data->normals = source.normals;
   data->uv_channels = source.uv_channels;
   data->bones = source.bones;
   data->morph_track = std::move(_data->morph_track);
   data->edges = source.edges;
   _data = std::move(data);

   _vertex_map.clear();
}

int32_t Geometry::getID() const
{
   return _id;
}

bool Geometry::isVisible() const
{
   return _visible;
}

void Geometry::setVisible(bool visible)
{
   _visible = visible;
}

bool Geometry::isMorphing() const
{
   return (_data->morph_track.size() > 0);
}

void Geometry::setMorphFrame(float frame)
{
   if (_data->morph_track.size() > 0)
   {
      _data->morph_track.get(_data->vertices, _data->normals, frame);
   }
}

void Geometry::calcBoundingBox(Vector& min, Vector& max)
{
   min = max = _data->vertices[0];
   for (const Vector& v : _data->vertices)
   {
      max.maximum(v);
      min.minimum(v);
   }
}

void Geometry::createQuad(float x, float y)
{
   static constexpr std::array<uint16_t, 6> indices = {0, 2, 1, 0, 3, 2};
   static const std::array<UV, 4> uvs = {UV(0.0f, 1.0f), UV(1.0f, 1.0f), UV(1.0f, 0.0f), UV(0.0f, 0.0f)};
   const std::array<Vector, 4> vertices = {Vector(0.0f, 0.0f, 0.0f), Vector(x, 0.0f, 0.0f), Vector(x, y, 0.0f), Vector(0.0f, y, 0.0f)};

   _data->indices.insert(_data->indices.end(), indices.begin(), indices.end());

   for (const auto& vertex : vertices)
   {
      _data->vertices.push_back(vertex);
      _data->normals.emplace_back(0.0f, 0.0f, 1.0f);
      _data->colors.emplace_back(1.0f, 1.0f, 1.0f);
   }

   _data->uv_channels.emplace_back(1, uvs);

   _vertex_map = {0, 1, 2, 3};
}

void Geometry::createCube(float scale)
{
   static constexpr std::array<uint16_t, 12 * 3> triangles = {
      0, 1, 2, 1, 3, 2,  // front
      2, 3, 6, 3, 7, 6,  // right
      4, 6, 5, 5, 6, 7,  // back
      0, 2, 4, 2, 6, 4,  // top
      0, 4, 5, 0, 5, 1,  // left
      1, 5, 7, 3, 1, 7   // bottom
   };

   static const std::array<Vector, 8> vertices = {
      Vector(-1.0f, -1.0f, -1.0f),
      Vector(-1.0f, 1.0f, -1.0f),
      Vector(1.0f, -1.0f, -1.0f),
      Vector(1.0f, 1.0f, -1.0f),

      Vector(-1.0f, -1.0f, 1.0f),
      Vector(-1.0f, 1.0f, 1.0f),
      Vector(1.0f, -1.0f, 1.0f),
      Vector(1.0f, 1.0f, 1.0f)
   };

   _data->indices.insert(_data->indices.end(), triangles.begin(), triangles.end());

   for (const auto& vertex : vertices)
   {
      _data->vertices.push_back(vertex * scale);
   }

   _vertex_map = {0, 1, 2, 3, 4, 5, 6, 7};
}

const int32_t* Geometry::getVertexMap() const
{
   return _vertex_map.empty() ? nullptr : _vertex_map.data();
}

const std::vector<uint16_t>& Geometry::getIndicesList() const
{
   return _data->indices;
}

const std::vector<Vector>& Geometry::getVertexList() const
{
   return _data->vertices;
}

const std::vector<Vector>& Geometry::getColorList() const
{
   return _data->colors;
}

const std::vector<Vector>& Geometry::getNormalList() const
{
   return _data->normals;
}

const std::vector<UVChannel>& Geometry::getUVList() const
{
   return _data->uv_channels;
}

const std::vector<Bone>& Geometry::getBoneList() const
{
   return _data->bones;
}

std::vector<Bone>& Geometry::getBoneList()
{
   return _data->bones;
}

const std::vector<Edge>& Geometry::getEdgeList() const
{
   return _data->edges;
}

void Geometry::load(Stream& stream)
{
   Chunk chunk(stream);
   Data& data = *_data;

   SceneGraph* scene = SceneGraph::instance();
   _material_id = chunk.getInt();
   if (_material_id >= 0 && scene)
   {
      _material_id += scene->getMaterialStartIndex();
   }

   loadList(chunk, data.vertices);
   loadList(chunk, data.normals);

   if (data.normals.empty())
   {
      calcNormals();
   }

   loadList(chunk, data.colors);
   loadList(chunk, data.uv_channels);
   loadFaceList(chunk, data.indices);
   loadList(chunk, data.bones);

   const int32_t rest = chunk.dataLeft();
   if (rest > 4)
   {
      data.morph_track << chunk;
      data.morph_track.calculateNormals(data.indices);
   }

   chunk.skip();
}

void Geometry::write(Stream& stream)
{
   Chunk chunk(stream, 100, "Geometry");
   Data& data = *_data;

   chunk.writeInt(_material_id);

   writeList(chunk, data.vertices);
   writeList(chunk, data.normals);
   writeList(chunk, data.colors);
   writeList(chunk, data.uv_channels);
   writeFaceList(chunk, data.indices);
   writeList(chunk, data.bones);
}

const Matrix& Geometry::getTransform() const
{
   if (_parent)
   {
      return _parent->getTransform();
   }
   return _identity;
}

int32_t Geometry::getMaterial() const
{
   return _material_id;
}

Node* Geometry::getParent() const
{
   return _parent;
}

void Geometry::setParent(Node* node)
{
   _parent = node;
}

int32_t Geometry::getIndexCount() const
{
   return static_cast<int32_t>(_data->indices.size());
}

uint16_t* Geometry::getIndices() const
{
   return _data->indices.empty() ? nullptr : _data->indices.data();
}

int32_t Geometry::getEdgeCount() const
{
   return static_cast<int32_t>(_data->edges.size());
}

Edge* Geometry::getEdges() const
{
   return _data->edges.empty() ? nullptr : _data->edges.data();
}

int32_t Geometry::getVertexCount() const
{
   return static_cast<int32_t>(_data->vertices.size());
}

Vector* Geometry::getVertices() const
{
   return _data->vertices.empty() ? nullptr : _data->vertices.data();
}

Vector* Geometry::getNormals() const
{
   return _data->normals.empty() ? nullptr : _data->normals.data();
}

Vector* Geometry::getColors() const
{
   return _data->colors.empty() ? nullptr : _data->colors.data();
}

UV* Geometry::getUV(int32_t channel) const
{
   for (UVChannel& uv_channel : _data->uv_channels)
   {
      if (uv_channel.id() == channel)
      {
         return uv_channel.data();
      }
   }
   return nullptr;
}

int32_t Geometry::getBoneCount() const
{
   return static_cast<int32_t>(_data->bones.size());
}

Bone* Geometry::getBones() const
{
   return _data->bones.empty() ? nullptr : _data->bones.data();
}

const Bone& Geometry::getBone(int32_t index) const
{
   return _data->bones[index];
}

void Geometry::calcNormals()
{
   Data& data = *_data;
   data.normals.assign(data.vertices.size(), Vector(0.0f, 0.0f, 0.0f));

   for (size_t i = 0; i + 2 < data.indices.size(); i += 3)
   {
      const uint16_t i1 = data.indices[i];
      const uint16_t i2 = data.indices[i + 1];
      const uint16_t i3 = data.indices[i + 2];

      const Vector& v1 = data.vertices[i1];
      const Vector& v2 = data.vertices[i2];
      const Vector& v3 = data.vertices[i3];

      const Vector normal = (v2 - v1) % (v3 - v1);

      data.normals[i1] += normal;
      data.normals[i2] += normal;
      data.normals[i3] += normal;
   }

   for (Vector& normal : data.normals)
   {
      normal.normalize();
   }
}

void Geometry::createBoxMapping(bool, const Vector& min, const Vector& max, const Matrix& transform, const Matrix& gizmo)
{
   const std::vector<uint16_t>& indices = _data->indices;
   const auto channel = std::ranges::find_if(_data->uv_channels, [](const UVChannel& uv_channel) { return uv_channel.id() == 1; });
   std::vector<UV>& uv = channel->getUV();

   std::vector<Vector> vertices;
   vertices.reserve(_data->vertices.size());
   for (const Vector& vertex : _data->vertices)
   {
      vertices.push_back(gizmo * (transform * vertex));
   }

   constexpr float su = 1.0f / 3.0f;
   constexpr float sv = 1.0f / 4.0f;

   for (size_t i = 0; i + 2 < indices.size(); i += 3)
   {
      const Vector& v1 = vertices[indices[i]];
      const Vector& v2 = vertices[indices[i + 1]];
      const Vector& v3 = vertices[indices[i + 2]];

      const Vector face_normal = (v2 - v1) % (v3 - v1);
      const int32_t axis = face_normal.absMaxIndex();

      for (size_t j = 0; j < 3; j++)
      {
         const int32_t index = indices[i + j];
         const Vector& vertex = vertices[index];

         switch (axis)
         {
            case 2:
               if (face_normal.z >= 0.0)  // top
               {
                  const float v = (max.x - vertex.x) / (max.x - min.x);
                  const float u = (max.y - vertex.y) / (max.y - min.y);
                  uv[index].u = u * su + su * 1;
                  uv[index].v = v * sv;
               }
               else  // bottom
               {
                  const float v = (vertex.x - min.x) / (max.x - min.x);
                  const float u = (vertex.y - min.y) / (max.y - min.y);
                  uv[index].u = u * su + su * 1;
                  uv[index].v = v * sv + sv * 2;
               }
               break;

            case 1:
               if (face_normal.y >= 0.0)  // right
               {
                  const float u = (max.x - vertex.x) / (max.x - min.x);
                  const float v = (max.z - vertex.z) / (max.z - min.z);
                  uv[index].u = u * su;
                  uv[index].v = v * sv + sv * 1;
               }
               else  // left
               {
                  const float u = (vertex.x - min.x) / (max.x - min.x);
                  const float v = (max.z - vertex.z) / (max.z - min.z);
                  uv[index].u = u * su + su * 2;
                  uv[index].v = v * sv + sv * 1;
               }
               break;

            case 0:
               if (face_normal.x >= 0.0)  // back
               {
                  const float u = (vertex.y - min.y) / (max.y - min.y);
                  const float v = (max.z - vertex.z) / (max.z - min.z);
                  uv[index].u = u * su + su * 1;
                  uv[index].v = v * sv + sv * 3;
               }
               else  // front
               {
                  const float v = (max.z - vertex.z) / (max.z - min.z);
                  const float u = (max.y - vertex.y) / (max.y - min.y);
                  uv[index].u = u * su + su * 1;
                  uv[index].v = v * sv + sv * 1;
               }
               break;

            default:
               break;
         }
      }
   }
}

std::vector<Vector> Geometry::getSkinVertices() const
{
   auto* mesh = static_cast<Mesh*>(_parent);
   MotionMixer* mixer = mesh->getMotionMixer();

   const std::vector<Vector>& vertices = _data->vertices;
   std::vector<Vector> skinned(vertices.size(), Vector(0.0f, 0.0f, 0.0f));

   for (const Bone& bone : _data->bones)
   {
      Node* node = mixer->getNode(bone.id());
      const Matrix& bone_matrix = node->getTransform();

      for (const Weight& weight : bone.weights())
      {
         const int32_t index = weight.id();
         skinned[index] += (bone_matrix * vertices[index]) * weight.weight();
      }
   }

   return skinned;
}
