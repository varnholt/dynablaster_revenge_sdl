// "geometry" is one part of a mesh,
// it is directly linked to its originating mesh-node by a parent pointer, but it's not a scenegraph-node on its own.
// a single mesh may be cut down to several geometry objects when using multiple materials or when exceeding the vertexlimit of 2^16

#pragma once

#include "animation/morphtrack.h"
#include "bone.h"
#include "edge.h"
#include "facelist.h"
#include "indexlist.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "tools/list.h"
#include "tools/referenced.h"
#include "uvchannel.h"

#include <cstdint>
#include <vector>

class Stream;
class Node;

class Geometry : public Referenced
{
public:
   Geometry(Node* parent);
   Geometry(const Geometry* geometry);

   void copy(const Geometry& geometry);

   void createQuad(float x, float y);
   void createCube(float scale);

   const FaceList& getIndicesList() const;
   const List<Vector>& getVertexList() const;
   const List<Vector>& getColorList() const;
   const List<Vector>& getNormalList() const;
   const List<UVChannel>& getUVList() const;
   const List<Bone>& getBoneList() const;
   const Array<Edge>& getEdgeList() const;
   const int32_t* getVertexMap() const;

   int32_t getID() const;  // unique id
   bool isVisible() const;
   void setVisible(bool visible);
   bool isMorphing() const;
   void setMorphFrame(float frame);  // interpolate vertex/normal data to given frame

   void load(Stream* stream);
   void write(Stream* stream);

   void setParent(Node* node);  // link geometry to originating mesh-node
   Node* getParent() const;     // originating mesh

   int32_t getIndexCount() const;  // number of indices (triangles*3)
   int32_t getVertexCount() const;
   int32_t getBoneCount() const;  // number of weighted vertices
   int32_t getEdgeCount() const;

   uint16_t* getIndices() const;

   Edge* getEdges() const;

   Vector* getVertices() const;
   Array<Vector> getSkinVertices() const;
   Bone* getBones() const;
   const Bone& getBone(int32_t index) const;
   Vector* getNormals() const;
   Vector* getColors() const;
   UV* getUV(int32_t channel) const;    // vertex-texcoord set "channel"
   int32_t getMaterial() const;         // material id
   const Matrix& getTransform() const;  // transformation matrix from mesh

   void calcBoundingBox(Vector& min, Vector& max);

   int32_t morphTargetCount() const;
   const List<Vector>& processMorphTrack(float time);

   void createBoxMapping(bool unwrap, const Vector& min, const Vector& max, const Matrix& tm = Matrix(), const Matrix& gizmo = Matrix());

private:
   int32_t createEdges();
   void calcNormals();

   int32_t _id = 0;
   Node* _parent = nullptr;  // link to mesh node
   bool _visible = true;
   int32_t _material_id = 0;

   FaceList _indices;  // polygon indices
   List<Vector> _vertices;
   List<Vector> _colors;
   List<Vector> _normals;
   List<UVChannel> _uv_channels;  // can be empty
   List<Bone> _bones;             // vertex weights
   MorphTrack _morph_track;
   Array<Edge> _edges;
   std::vector<int32_t> _vertex_map;  // vertex identity map
};
