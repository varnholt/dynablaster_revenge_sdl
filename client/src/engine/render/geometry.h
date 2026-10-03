// "geometry" is one part of a mesh,
// it is directly linked to its originating mesh-node by a parent pointer, but it's not a scenegraph-node on its own.
// a single mesh may be cut down to several geometry objects when using multiple materials or when exceeding the vertexlimit of 2^16

#pragma once

#include "animation/morphtrack.h"
#include "bone.h"
#include "edge.h"
#include "facelist.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "uvchannel.h"

#include <cstdint>
#include <memory>
#include <vector>

class Stream;
class Node;

class Geometry
{
public:
   Geometry(Node* parent);

   // copies share id and vertex data (and with that the vertex buffer of the materials)
   Geometry(const Geometry& geometry) = default;

   // deep copy of the vertex data of "geometry"
   void copy(const Geometry& geometry);

   void createQuad(float x, float y);
   void createCube(float scale);

   const std::vector<uint16_t>& getIndicesList() const;
   const std::vector<Vector>& getVertexList() const;
   const std::vector<Vector>& getColorList() const;
   const std::vector<Vector>& getNormalList() const;
   const std::vector<UVChannel>& getUVList() const;
   const std::vector<Bone>& getBoneList() const;
   std::vector<Bone>& getBoneList();
   const std::vector<Edge>& getEdgeList() const;
   const int32_t* getVertexMap() const;

   int32_t getID() const;  // unique id
   bool isVisible() const;
   void setVisible(bool visible);
   bool isMorphing() const;
   void setMorphFrame(float frame);  // interpolate vertex/normal data to given frame

   void load(Stream& stream);
   void write(Stream& stream);

   void setParent(Node* node);  // link geometry to originating mesh-node
   Node* getParent() const;     // originating mesh

   int32_t getIndexCount() const;  // number of indices (triangles*3)
   int32_t getVertexCount() const;
   int32_t getBoneCount() const;  // number of weighted vertices
   int32_t getEdgeCount() const;

   uint16_t* getIndices() const;

   Edge* getEdges() const;

   Vector* getVertices() const;
   std::vector<Vector> getSkinVertices() const;
   Bone* getBones() const;
   const Bone& getBone(int32_t index) const;
   Vector* getNormals() const;
   Vector* getColors() const;
   UV* getUV(int32_t channel) const;    // vertex-texcoord set "channel"
   int32_t getMaterial() const;         // material id
   const Matrix& getTransform() const;  // transformation matrix from mesh

   void calcBoundingBox(Vector& min, Vector& max);

   void createBoxMapping(bool unwrap, const Vector& min, const Vector& max, const Matrix& tm = Matrix(), const Matrix& gizmo = Matrix());

private:
   struct Data
   {
      std::vector<uint16_t> indices;  // polygon indices
      std::vector<Vector> vertices;
      std::vector<Vector> colors;
      std::vector<Vector> normals;
      std::vector<UVChannel> uv_channels;  // can be empty
      std::vector<Bone> bones;             // vertex weights
      MorphTrack morph_track;
      std::vector<Edge> edges;
   };

   void calcNormals();

   int32_t _id = 0;
   Node* _parent = nullptr;  // link to mesh node
   bool _visible = true;
   int32_t _material_id = 0;

   std::shared_ptr<Data> _data = std::make_shared<Data>();
   std::vector<int32_t> _vertex_map;  // vertex identity map
};
