#include "node.h"
#include <algorithm>
#include "scenegraph.h"
#include "tools/stream.h"

Node::Node(Node::ID id, Node* parent) : _id(id)
{
   setParent(parent);
}

Node::Node(const Node& node, Node* parent)
    : ObjectName(node),
      _id(node.id()),
      _has_skinning(node.hasSkinning()),
      _user_transform(node.getUserTransformable()),
      _frame(node.getFrame()),
      _position_track(node.getPositionTrack()),
      _rotation_track(node.getRotationTrack()),
      _scale_track(node.getScaleTrack()),
      _flip_track(node.getFlipTrack()),
      _visibility_track(node.getVisibilityTrack()),
      _bake(node.getBakedAnimation())
{
   if (parent)
   {
      setParent(parent);
   }
   else
   {
      setParent(node.parent());
   }
}

Node::~Node()
{
   SceneGraph* scene = getRoot();
   if (scene && scene != this)
   {
      scene->removeNode(this);
   }
}

// get root node (scene)
SceneGraph* Node::getRoot() const
{
   const Node* root = this;
   while (root->parent())
   {
      root = root->parent();
   }

   if (root->id() == Node::idRoot)
   {
      return static_cast<SceneGraph*>(const_cast<Node*>(root));
   }
   return nullptr;
}

Node::ID Node::id() const
{
   return _id;
}

void Node::addChild(Node* node)
{
   _children.push_back(node);
   Node* root = this;
   while (root && root->id() != idRoot)
   {
      root = root->parent();
   }
   if (root)
   {
      static_cast<SceneGraph*>(root)->addNode(node);
   }
}

int32_t Node::getChildCount() const
{
   return static_cast<int32_t>(_children.size());
}

Node* Node::getChild(int32_t index) const
{
   if (index >= 0 && index < getChildCount())
   {
      return _children[index];
   }
   return nullptr;
}

Node* Node::getChild(const String& name) const
{
   const auto child = std::ranges::find_if(_children, [&name](const Node* node) { return node->name() == name; });
   return (child != _children.end()) ? *child : nullptr;
}

bool Node::visible() const
{
   return _visible;
}

void Node::setVisible(bool visible)
{
   _visible = visible;
}

bool Node::hasSkinning() const
{
   return _has_skinning;
}

float Node::getFrame() const
{
   return _frame;
}

void Node::setFrame(float frame)
{
   _frame = frame;
}

int32_t Node::getAnimationLength() const
{
   return std::max({0, _position_track.getAnimationLength(), _rotation_track.getAnimationLength(), _scale_track.getAnimationLength()});
}

void Node::setParent(Node* parent)
{
   _parent = parent;
   if (parent)
   {
      parent->addChild(this);
   }
}

Node* Node::parent() const
{
   return _parent;
}

int32_t Node::getDepth() const
{
   int32_t depth = 0;
   const Node* node = this;
   while (node)
   {
      node = node->parent();
      depth++;
   }
   return depth;
}

void Node::load(Stream*)
{
}

void Node::setTransform(const Matrix& matrix)
{
   _transform = matrix;
}

const Matrix& Node::getTransform() const
{
   return _transform;
}

void Node::write(Stream*)
{
}

Vector Node::getPosition() const
{
   return _transform.translation();
}

const PosTrack& Node::getPositionTrack() const
{
   return _position_track;
}

const RotTrack& Node::getRotationTrack() const
{
   return _rotation_track;
}

const ScaleTrack& Node::getScaleTrack() const
{
   return _scale_track;
}

const ValTrack& Node::getFlipTrack() const
{
   return _flip_track;
}

const VisTrack& Node::getVisibilityTrack() const
{
   return _visibility_track;
}

void Node::transform(float time)
{
   // in user-transform mode the matrix has already been set from outside
   if (_user_transform)
   {
      return;
   }

   time += getFrame();

   if (_bake.size() == 0)
   {
      const Vector position = _position_track.get(time);
      const Quat rotation = _rotation_track.get(time);
      const Matrix stm = _scale_track.get(time);

      // the flip track is deprecated, flipping is always disabled
      const float flip = 1.0f;

      Matrix ptm;
      ptm.translate(position);

      const Matrix rtm(rotation);

      const Matrix ftm(Vector(flip, flip, flip));

      _transform = stm * rtm * ftm * ptm;

      if (parent())
      {
         _transform = _transform * parent()->getTransform();
      }
   }
   else
   {
      _transform = _bake.interpolate(time) * parent()->getTransform();
   }
}

void Node::setUserTransformable(bool state)
{
   _user_transform = state;
}

bool Node::getUserTransformable() const
{
   return _user_transform;
}

const BakedTransformation& Node::getBakedAnimation() const
{
   return _bake;
}

void Node::bakeAnimationTrack(float step)
{
   _bake = BakedTransformation(this, step);
}
