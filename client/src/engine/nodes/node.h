// node-class
// abstract base class of all scene-objects

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"
#include "tools/objectname.h"
#include "tools/streamable.h"

#include "animation/bakedtransformation.h"
#include "animation/postrack.h"
#include "animation/rottrack.h"
#include "animation/scaletrack.h"
#include "animation/valtrack.h"
#include "animation/vistrack.h"

class SceneGraph;

// every node of a scene is owned by the SceneGraph's flat node list, which also links it to its
// parent; parent and children only refer to each other
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

   explicit Node(ID id);
   Node(const Node& node);  // copies name, tracks and flags, not the links to parent and children
   Node& operator=(const Node&) = delete;
   ~Node() override = default;
   ID id() const;                                               // return the node id (object type)
   std::optional<std::reference_wrapper<Node>> parent() const;  // get parent node
   std::optional<std::reference_wrapper<SceneGraph>> getRoot();
   int32_t getDepth() const;
   bool visible() const;
   void setVisible(bool visible);
   int32_t getChildCount() const;
   Node& getChild(int32_t index) const;
   std::optional<std::reference_wrapper<Node>> findChild(const std::string& name) const;
   bool getUserTransformable() const;
   void setUserTransformable(bool state);
   bool hasSkinning() const;
   float getFrame() const;
   void setFrame(float frame);
   int32_t getAnimationLength() const;
   void bakeAnimationTrack(float step);
   const BakedTransformation& getBakedAnimation() const;

   void load(Stream& stream) override;  // load object from stream
   void write(Stream& stream) override;

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
   friend class SceneGraph;

   void linkChild(Node& child);
   void unlinkChild(const Node& child);

   ID _id;                                               // pseudo-rtti to identify the object type
   std::optional<std::reference_wrapper<Node>> _parent;  // parent object
   std::vector<std::reference_wrapper<Node>> _children;  // child objects

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
