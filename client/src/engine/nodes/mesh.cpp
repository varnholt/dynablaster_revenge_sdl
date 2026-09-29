#include "mesh.h"
#include "animation/motionmixer.h"
#include "math/vector.h"
#include "renderdevice.h"
#include "tools/stream.h"

Mesh::Mesh(Node* parent) : Node(Node::idMesh, parent)
{
}

Mesh::Mesh(const Mesh& mesh, Node* parent) : Node(mesh, parent), _skeleton(mesh.getSkeleton()), _motion_mixer(mesh.getMotionMixer())
{
   for (int32_t i = 0; i < mesh.getPartCount(); i++)
   {
      Geometry* geometry = new Geometry(*mesh.getPart(i));
      geometry->setParent(this);
      add(geometry);
   }

   setUserTransformable(true);
}

void Mesh::copy(const Mesh& mesh)
{
   if (!_parent)
   {
      _parent = mesh.parent();
   }
   _has_skinning = mesh.hasSkinning();
   _user_transform = mesh.getUserTransformable();
   _frame = mesh.getFrame();
   _animation_frame = mesh.getAnimationFrame();
   _skeleton = mesh.getSkeleton();
   _motion_mixer = mesh.getMotionMixer();

   _bake = mesh.getBakedAnimation();

   for (int32_t i = 0; i < mesh.getPartCount(); i++)
   {
      Geometry* geometry = new Geometry(this);
      geometry->copy(*(mesh.getPart(i)));
      add(geometry);
   }
}

void Mesh::setAnimationFrame(float frame)
{
   _animation_frame = frame;
}

float Mesh::getAnimationFrame() const
{
   return _animation_frame;
}

void Mesh::add(Geometry* geometry)
{
   _geometry.push_back(geometry);
}

int32_t Mesh::getPartCount() const
{
   return static_cast<int32_t>(_geometry.size());
}

Geometry* Mesh::getPart(int32_t index) const
{
   return _geometry[index];
}

Node* Mesh::getSkeleton() const
{
   return _skeleton;
}

void Mesh::setSkeleton(Node* node)
{
   _skeleton = node;
}

MotionMixer* Mesh::getMotionMixer() const
{
   return _motion_mixer;
}

void Mesh::setMotionMixer(MotionMixer* mixer)
{
   _motion_mixer = mixer;
}

void Mesh::transform(float frame)
{
   Node::transform(frame);
   MotionMixer* mixer = getMotionMixer();
   if (!mixer)
   {
      return;
   }

   mixer->animate(getFrame());
}

uint32_t Mesh::getRenderFlags() const
{
   return _render_flags;
}

void Mesh::setRenderFlags(uint32_t flags)
{
   _render_flags = flags;
}

float Mesh::getRenderParameter(int32_t index) const
{
   return _render_parameter[index];
}

void Mesh::setRenderParameter(int32_t index, float param)
{
   _render_parameter[index] = param;
}

void Mesh::load(Stream* stream)
{
   Node::load(stream);

   // flags (cast/receive shadow) are not used
   stream->getInt();

   const int32_t geometry_count = stream->getInt();
   _geometry.clear();
   _geometry.reserve(geometry_count);
   for (int32_t i = 0; i < geometry_count; i++)
   {
      Geometry* geometry = new Geometry(this);
      geometry->load(stream);
      _geometry.push_back(geometry);
   }

   // read tracks
   Chunk animation(stream);
   _position_track.load(&animation);
   _rotation_track.load(&animation);
   _scale_track.load(&animation);
   _visibility_track.load(&animation);
   animation.skip();
}

void Mesh::write(Stream* stream)
{
   Node::write(stream);

   const int32_t flags = 0;  // TODO!
   stream->writeInt(flags);

   stream->writeInt(getPartCount());
   for (Geometry* geometry : _geometry)
   {
      geometry->write(stream);
   }

   // write tracks
   Chunk animation(stream, 2000, "Animation");
   _position_track.write(&animation);
   _rotation_track.write(&animation);
   _scale_track.write(&animation);
   _visibility_track.write(&animation);
   _flip_track.write(&animation);
}

void Mesh::createBoxMapping(bool unwrap, const Vector& min, const Vector& max, const Matrix& gizmo)
{
   for (Geometry* geometry : _geometry)
   {
      geometry->createBoxMapping(unwrap, min, max, getTransform(), gizmo);
   }
}
