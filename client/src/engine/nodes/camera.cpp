#include "camera.h"
#include "tools/stream.h"

Camera::Camera(Node* parent) : Node(Node::idCamera, parent)
{
}

void Camera::load(Stream* stream)
{
   Node::load(stream);

   // read tracks
   Chunk animation(stream);
   _fov_track.load(&animation);
   _position_track.load(&animation);
   _rotation_track.load(&animation);
   _scale_track.load(&animation);
   _visibility_track.load(&animation);
   _flip_track.load(&animation);
   animation.skip();
}

void Camera::write(Stream* stream)
{
   Node::write(stream);

   // write tracks
   Chunk animation(stream, 2000, "Animation");
   _fov_track.write(&animation);
   _position_track.write(&animation);
   _rotation_track.write(&animation);
   _scale_track.write(&animation);
   _visibility_track.write(&animation);
   _flip_track.write(&animation);
}

void Camera::transform(float time)
{
   Node::transform(time);
   if (!_user_transform)
   {
      _fov = _fov_track.get(time);
   }
}

float Camera::getFOV() const
{
   return _fov;
}

void Camera::setFOV(float fov)
{
   _fov = fov;
}

float Camera::getNear() const
{
   return _near;
}

void Camera::setNear(float z_near)
{
   _near = z_near;
}

float Camera::getFar() const
{
   return _far;
}

void Camera::setFar(float z_far)
{
   _far = z_far;
}

void Camera::setPerspectiveMode(bool mode)
{
   _perspective_mode = mode;
}

bool Camera::getPerspectiveMode() const
{
   return _perspective_mode;
}

void Camera::lookAt(const Vector& position, const Vector& target, const Vector& up)
{
   const Matrix matrix = Matrix::lookAt(position, target, up);

   setTransform(matrix.invert());
}
