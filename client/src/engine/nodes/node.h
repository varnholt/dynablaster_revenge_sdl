// node-class
// abstract base class of all scene-objects

#pragma once

#include <cstdint>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"
#include "tools/array.h"
#include "tools/objectname.h"
#include "tools/streamable.h"

#include "animation/bakedtransformation.h"
#include "animation/postrack.h"
#include "animation/rottrack.h"
#include "animation/scaletrack.h"
#include "animation/valtrack.h"
#include "animation/vistrack.h"

class SceneGraph;

// a node's parent does not own it: every node in a scene is owned by the SceneGraph's flat node
// list, and a node unregisters itself from that list on destruction
class Node : public Streamable, public ObjectName
{
public:
   enum ID
   {
      idRoot = -1,
      idNone = 0,
      idMesh = 1,
      idDummy = 2,
      idShape = 3,
      idCamera = 4,
      idDir = 9,
      idOmni = 10,
      idSpot = 11,
      idAnimMesh = 32
   };

   Node(ID id, Node* parent = nullptr);
   Node(const Node& node, Node* parent = nullptr);
   ~Node() override;
   ID id() const;         // return the node id (object type)
   Node* parent() const;  // get parent node
   SceneGraph* getRoot() const;
   int32_t getDepth() const;
   bool visible() const;
   void setVisible(bool visible);
   void addChild(Node* node);
   int32_t getChildCount() const;
   Node* getChild(int32_t index) const;
   Node* getChild(const String& name) const;
   void setParent(Node* parent);  // link to parent obj. includes this to parent's children
   bool getUserTransformable() const;
   void setUserTransformable(bool state);
   bool hasSkinning() const;
   float getFrame() const;
   void setFrame(float frame);
   int32_t getAnimationLength() const;
   void bakeAnimationTrack(float step);
   const BakedTransformation& getBakedAnimation() const;

   void load(Stream* stream) override;  // load object from stream
   void write(Stream* stream) override;

   virtual void transform(float frame);  // calc transformation at frame

   void setTransform(const Matrix& matrix);
   const Matrix& getTransform() const;
   Vector getPosition() const;

   const PosTrack& getPositionTrack() const;
   const RotTrack& getRotationTrack() const;
   const ScaleTrack& getScaleTrack() const;
   const ValTrack& getFlipTrack() const;
   const VisTrack& getVisibilityTrack() const;

protected:
   ID _id;                        // pseudo-rtti to identify the object type
   Node* _parent = nullptr;       // parent object
   std::vector<Node*> _children;  // child objects (not owned)

   Matrix _transform;
   bool _has_skinning = false;
   bool _visible = true;

   bool _user_transform = false;
   float _frame = 0.0f;

   // tracks
   PosTrack _position_track;
   RotTrack _rotation_track;
   ScaleTrack _scale_track;
   ValTrack _flip_track;  // deprecated
   VisTrack _visibility_track;
   BakedTransformation _bake;
};
