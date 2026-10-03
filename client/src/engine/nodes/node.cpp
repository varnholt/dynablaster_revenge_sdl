#include "node.h"
#include <algorithm>
#include "scenegraph.h"
#include "tools/stream.h"

Node::Node(Node::ID id) : _id(id)
{
}

Node::Node(const Node& node)
    : Streamable(node),
      ObjectName(node),
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
}

// get root node (scene)
std::optional<std::reference_wrapper<SceneGraph>> Node::getRoot()
{
   std::reference_wrapper<Node> root = *this;
   while (root.get()._parent)
   {
      root = *root.get()._parent;
   }

   if (root.get().id() == Node::idRoot)
   {
      return static_cast<SceneGraph&>(root.get());
   }
   return std::nullopt;
}

Node::ID Node::id() const
{
   return _id;
}

void Node::linkChild(Node& child)
{
   child._parent = *this;
   _children.emplace_back(child);
}

void Node::unlinkChild(const Node& child)
{
   std::erase_if(_children, [&child](const Node& candidate) { return &candidate == &child; });
}

int32_t Node::getChildCount() const
{
   return static_cast<int32_t>(_children.size());
}

Node& Node::getChild(int32_t index) const
{
   return _children[index];
}

std::optional<std::reference_wrapper<Node>> Node::findChild(const std::string& name) const
{
   const auto child = std::ranges::find_if(_children, [&name](const Node& node) { return node.name() == name; });
   if (child != _children.end())
   {
      return *child;
   }
   return std::nullopt;
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

std::optional<std::reference_wrapper<Node>> Node::parent() const
{
   return _parent;
}

int32_t Node::getDepth() const
{
   int32_t depth = 1;
   for (auto node = _parent; node; node = node->get()._parent)
   {
      depth++;
   }
   return depth;
}

void Node::load(Stream&)
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

void Node::write(Stream&)
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

      if (_parent)
      {
         _transform = _transform * _parent->get().getTransform();
      }
   }
   else
   {
      _transform = _bake.interpolate(time) * _parent.value().get().getTransform();
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
   _bake = BakedTransformation(*this, step);
}
