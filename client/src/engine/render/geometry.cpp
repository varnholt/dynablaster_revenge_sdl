#include "geometry.h"
#include "animation/motionmixer.h"
#include "math/vector.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "nodes/scenegraph.h"
#include "tools/profiling.h"
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

Geometry::Geometry(const Geometry* geometry)
    : Referenced(*geometry),
      _id(geometry->getID()),
      _parent(geometry->getParent()),
      _visible(geometry->isVisible()),
      _material_id(-1),
      _vertex_map(geometry->_vertex_map)
{
   _indices = geometry->getIndicesList();
   _vertices = geometry->getVertexList();
   _colors = geometry->getColorList();
   _normals = geometry->getNormalList();
   _uv_channels = geometry->getUVList();
   _bones = geometry->getBoneList();
   _edges = geometry->getEdgeList();
}

void Geometry::copy(const Geometry& geometry)
{
   _indices.copy(geometry.getIndicesList());
   _vertices.copy(geometry.getVertexList());
   _colors.copy(geometry.getColorList());
   _normals.copy(geometry.getNormalList());

   const List<UVChannel>& uv_channels = geometry.getUVList();
   _uv_channels.init(uv_channels.size());
   for (int32_t i = 0; i < uv_channels.size(); i++)
   {
      UVChannel channel;
      channel.copy(uv_channels[i]);
      _uv_channels.add(channel);
   }

   _bones.copy(geometry.getBoneList());
   _edges.copy(geometry.getEdgeList());

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
   return (_morph_track.size() > 0);
}

void Geometry::setMorphFrame(float frame)
{
   if (_morph_track.size() > 0)
   {
      _morph_track.get(_vertices, _normals, frame);
   }
}

void Geometry::calcBoundingBox(Vector& min, Vector& max)
{
   min = max = _vertices[0];
   for (int32_t i = 0; i < _vertices.size(); i++)
   {
      const Vector& v = _vertices[i];
      max.maximum(v);
      min.minimum(v);
   }
}

void Geometry::createQuad(float x, float y)
{
   static constexpr std::array<uint16_t, 6> indices = {0, 2, 1, 0, 3, 2};
   static std::array<UV, 4> uvs = {UV(0.0f, 1.0f), UV(1.0f, 1.0f), UV(1.0f, 0.0f), UV(0.0f, 0.0f)};
   const std::array<Vector, 4> vertices = {Vector(0.0f, 0.0f, 0.0f), Vector(x, 0.0f, 0.0f), Vector(x, y, 0.0f), Vector(0.0f, y, 0.0f)};

   for (const auto index : indices)
   {
      _indices.add(index);
   }

   for (const auto& vertex : vertices)
   {
      _vertices.add(vertex);
      _normals.add(Vector(0.0f, 0.0f, 1.0f));
      _colors.add(Vector(1.0f, 1.0f, 1.0f));
   }

   _uv_channels.add(UVChannel(1, uvs.data(), static_cast<int32_t>(uvs.size())));

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

   for (const auto index : triangles)
   {
      _indices.add(index);
   }

   for (const auto& vertex : vertices)
   {
      _vertices.add(vertex * scale);
   }

   _vertex_map = {0, 1, 2, 3, 4, 5, 6, 7};
}

const int32_t* Geometry::getVertexMap() const
{
   return _vertex_map.empty() ? nullptr : _vertex_map.data();
}

const FaceList& Geometry::getIndicesList() const
{
   return _indices;
}

const List<Vector>& Geometry::getVertexList() const
{
   return _vertices;
}

const List<Vector>& Geometry::getColorList() const
{
   return _colors;
}

const List<Vector>& Geometry::getNormalList() const
{
   return _normals;
}

const List<UVChannel>& Geometry::getUVList() const
{
   return _uv_channels;
}

const List<Bone>& Geometry::getBoneList() const
{
   return _bones;
}

const Array<Edge>& Geometry::getEdgeList() const
{
   return _edges;
}

int32_t Geometry::createEdges()
{
   std::array<uint16_t, 3> edge{};                      // matching edge-vertices
   const int32_t triangle_count = _indices.size() / 3;  // 3 indices per triangle
   const int32_t vertex_count = _vertices.size();
   const int32_t* vertex_map = _vertex_map.data();

   double time = getCpuTick();

   int32_t unique_vertex_count = 0;
   for (int32_t i = 0; i < vertex_count; i++)
   {
      unique_vertex_count = std::max(unique_vertex_count, vertex_map[i]);
   }
   unique_vertex_count++;

   // polygon normals
   std::vector<Vector> normals(triangle_count);
   const uint16_t* poly = _indices.data();
   for (int32_t i = 0; i < triangle_count; i++)
   {
      const Vector& v1 = _vertices[*poly++];
      const Vector& v2 = _vertices[*poly++];
      const Vector& v3 = _vertices[*poly++];
      normals[i] = (v2 - v1) % (v3 - v1);
      normals[i].normalize();
   }

   // for each vertex: list of connected faces
   std::vector<IndexList> vertex_connected_faces(unique_vertex_count);
   poly = _indices.data();
   for (int32_t i = 0; i < triangle_count; i++)
   {
      for (int32_t j = 0; j < 3; j++)
      {
         vertex_connected_faces[vertex_map[*poly++]].add(i);
      }
   }

   // for each face: list of connected faces
   std::vector<IndexList> face_connected_faces(triangle_count);
   poly = _indices.data();
   for (int32_t i = 0; i < triangle_count; i++)
   {
      IndexList& face_list = face_connected_faces[i];
      for (int32_t j = 0; j < 3; j++)
      {
         // faces connected to current vertex j of this triangle
         face_list.merge(vertex_connected_faces[vertex_map[*poly++]]);
      }
   }

   // for each vertex: list of connected vertices, thus building an edge
   std::vector<IndexList> vertex_connections(unique_vertex_count);

   _edges.init(triangle_count * 3);  // maximum number of edges is 3 per triangle
   poly = _indices.data();
   for (int32_t i = 0; i < triangle_count; i++, poly += 3)
   {
      const int32_t va1 = poly[0];
      const int32_t va2 = poly[1];
      const int32_t va3 = poly[2];

      const int32_t p1 = vertex_map[va1];
      const int32_t p2 = vertex_map[va2];
      const int32_t p3 = vertex_map[va3];

      const IndexList& face_list = face_connected_faces[i];
      for (int32_t j = 0; j < face_list.size(); j++)
      {
         const int32_t face_id = face_list.get(j);
         const uint16_t* test = _indices.data() + face_id * 3;
         int32_t count = 0;

         const int32_t vb1 = test[0];
         const int32_t vb2 = test[1];
         const int32_t vb3 = test[2];

         if (vertex_map[vb1] == p1 || vertex_map[vb2] == p1 || vertex_map[vb3] == p1)
         {
            edge[count++] = static_cast<uint16_t>(va1);
         }
         if (vertex_map[vb1] == p2 || vertex_map[vb2] == p2 || vertex_map[vb3] == p2)
         {
            edge[count++] = static_cast<uint16_t>(va2);
         }
         if (vertex_map[vb1] == p3 || vertex_map[vb2] == p3 || vertex_map[vb3] == p3)
         {
            edge[count++] = static_cast<uint16_t>(va3);
         }

         if (count != 2)
         {
            continue;
         }

         if ((edge[0] == va1 && edge[1] == va3) || (edge[0] == va2 && edge[1] == va1) || (edge[0] == va3 && edge[1] == va2))
         {
            const uint16_t t = edge[2];
            edge[2] = edge[0];
            edge[0] = edge[1];
            edge[1] = t;
         }

         const int32_t e0 = vertex_map[edge[0]];
         const int32_t e1 = vertex_map[edge[1]];

         // skip if already connected
         if (vertex_connections[e0].find(e1) || vertex_connections[e1].find(e0))
         {
            continue;
         }

         vertex_connections[e0].add(e1);
         vertex_connections[e1].add(e0);

         // compare assigned normals
         int32_t n1 = 0;
         int32_t n2 = 0;
         for (int32_t k = 0; k < 3; k++)
         {
            const int32_t index = test[k];
            if (e0 == vertex_map[index])
            {
               n1 = index;
            }
            if (e1 == vertex_map[index])
            {
               n2 = index;
            }
         }

         int32_t flags = 1;
         if ((_normals[n1] == _normals[edge[0]]) && (_normals[n2] == _normals[edge[1]]))
         {
            flags = 0;
         }

         // get "unused" vertex from each triangle
         int32_t vc = -1;
         if (e0 != vertex_map[va1] && e1 != vertex_map[va1])
         {
            vc = va1;
         }
         if (e0 != vertex_map[va2] && e1 != vertex_map[va2])
         {
            vc = va2;
         }
         if (e0 != vertex_map[va3] && e1 != vertex_map[va3])
         {
            vc = va3;
         }

         int32_t vd = -1;
         if (e0 != vertex_map[vb1] && e1 != vertex_map[vb1])
         {
            vd = vb1;
         }
         if (e0 != vertex_map[vb2] && e1 != vertex_map[vb2])
         {
            vd = vb2;
         }
         if (e0 != vertex_map[vb3] && e1 != vertex_map[vb3])
         {
            vd = vb3;
         }

         if (vc != -1 && vd != -1)
         {
            Edge e;
            e.i1 = edge[0];
            e.i2 = edge[1];
            e.i3 = static_cast<uint16_t>(vc);
            e.i4 = static_cast<uint16_t>(vd);
            e.f1 = i;
            e.f2 = face_id;
            e.flags = flags;
            _edges.add(e);
         }
      }
   }

   int32_t unconnected = 0;
   poly = _indices.data();
   for (int32_t i = 0; i < triangle_count; i++, poly += 3)
   {
      const std::array<int32_t, 5> index = {poly[0], poly[1], poly[2], poly[0], poly[1]};

      for (int32_t j = 0; j < 3; j++)
      {
         const int32_t m1 = vertex_map[index[j]];
         const int32_t m2 = vertex_map[index[j + 1]];
         if (!vertex_connections[m1].find(m2) && !vertex_connections[m2].find(m1))
         {
            vertex_connections[m1].add(m2);
            vertex_connections[m2].add(m1);
            unconnected++;
         }
      }
   }

   time = getCpuTick() - time;

   return _edges.size();
}

void Geometry::load(Stream* stream)
{
   Chunk chunk(stream);

   SceneGraph* scene = SceneGraph::instance();
   _material_id = chunk.getInt();
   if (_material_id >= 0 && scene)
   {
      _material_id += scene->getMaterialStartIndex();
   }

   _vertices << chunk;
   _normals << chunk;

   if (_normals.size() == 0)
   {
      calcNormals();
   }

   _colors << chunk;
   _uv_channels << chunk;
   _indices << chunk;
   _bones << chunk;

   const int32_t rest = chunk.dataLeft();
   if (rest > 4)
   {
      _morph_track << chunk;
      _morph_track.calculateNormals(_indices);
   }

   chunk.skip();
}

void Geometry::write(Stream* stream)
{
   Chunk chunk(stream, 100, "Geometry");

   chunk.writeInt(_material_id);

   _vertices >> chunk;
   _normals >> chunk;
   _colors >> chunk;
   _uv_channels >> chunk;
   _indices >> chunk;
   _bones >> chunk;
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
   return _indices.size();
}

uint16_t* Geometry::getIndices() const
{
   return _indices.data();
}

int32_t Geometry::getEdgeCount() const
{
   return _edges.size();
}

Edge* Geometry::getEdges() const
{
   return _edges.data();
}

int32_t Geometry::getVertexCount() const
{
   return _vertices.size();
}

Vector* Geometry::getVertices() const
{
   return _vertices.data();
}

Vector* Geometry::getNormals() const
{
   return _normals.data();
}

Vector* Geometry::getColors() const
{
   return _colors.data();
}

UV* Geometry::getUV(int32_t channel) const
{
   const int32_t count = _uv_channels.size();
   for (int32_t i = 0; i < count; i++)
   {
      const UVChannel& uv_channel = _uv_channels[i];
      if (uv_channel.id() == channel)
      {
         return uv_channel.data();
      }
   }
   return nullptr;
}

int32_t Geometry::getBoneCount() const
{
   return _bones.size();
}

Bone* Geometry::getBones() const
{
   return _bones.data();
}

const Bone& Geometry::getBone(int32_t index) const
{
   return _bones[index];
}

void Geometry::calcNormals()
{
   _normals.init(_vertices.size());

   for (int32_t i = 0; i < _vertices.size(); i++)
   {
      _normals.add(Vector(0.0f, 0.0f, 0.0f));
   }

   const uint16_t* index = _indices.data();

   for (int32_t i = 0; i < _indices.size(); i += 3)
   {
      const uint16_t i1 = *index++;
      const uint16_t i2 = *index++;
      const uint16_t i3 = *index++;

      const Vector& v1 = _vertices[i1];
      const Vector& v2 = _vertices[i2];
      const Vector& v3 = _vertices[i3];

      const Vector normal = (v2 - v1) % (v3 - v1);

      _normals[i1] += normal;
      _normals[i2] += normal;
      _normals[i3] += normal;
   }

   for (int32_t i = 0; i < _normals.size(); i++)
   {
      _normals[i].normalize();
   }
}

void Geometry::createBoxMapping(bool, const Vector& min, const Vector& max, const Matrix& transform, const Matrix& gizmo)
{
   UV* uv = getUV(1);

   std::vector<Vector> vertices(_vertices.size());
   for (int32_t i = 0; i < _vertices.size(); i++)
   {
      vertices[i] = gizmo * (transform * _vertices[i]);
   }

   constexpr float su = 1.0f / 3.0f;
   constexpr float sv = 1.0f / 4.0f;

   for (int32_t i = 0; i < _indices.size(); i += 3)
   {
      const Vector& v1 = vertices[_indices[i]];
      const Vector& v2 = vertices[_indices[i + 1]];
      const Vector& v3 = vertices[_indices[i + 2]];

      const Vector face_normal = (v2 - v1) % (v3 - v1);
      const int32_t axis = face_normal.absMaxIndex();

      for (int32_t j = 0; j < 3; j++)
      {
         const int32_t index = _indices[i + j];
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

Array<Vector> Geometry::getSkinVertices() const
{
   auto* mesh = static_cast<Mesh*>(_parent);
   MotionMixer* mixer = mesh->getMotionMixer();

   const int32_t vertex_count = getVertexCount();
   Array<Vector> result(vertex_count);

   const Vector* vertices = getVertices();
   Vector* skinned = result.data();
   std::ranges::fill(std::span(skinned, static_cast<size_t>(vertex_count)), Vector(0.0f, 0.0f, 0.0f));

   for (int32_t b = 0; b < getBoneCount(); b++)
   {
      const Bone& bone = getBone(b);

      Node* node = mixer->getNode(bone.id());
      const Matrix& bone_matrix = node->getTransform();

      const Weight* weights = bone.weights();
      for (int32_t v = 0; v < bone.count(); v++)
      {
         const int32_t index = weights[v].id();
         const float factor = weights[v].weight();

         skinned[index] += (bone_matrix * vertices[index]) * factor;
      }
   }

   return result;
}
