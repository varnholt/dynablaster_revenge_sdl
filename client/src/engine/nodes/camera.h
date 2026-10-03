// camera object
// the camera's transformation defines the view matrix (additionally requires fov scaling)

#pragma once

#include "animation/valtrack.h"
#include "node.h"

class Stream;

class Camera : public Node
{
public:
   Camera(Node* parent = nullptr);
   void load(Stream& stream) override;
   void write(Stream& stream) override;

   void transform(float time) override;
   float getFOV() const;
   void setFOV(float fov);
   float getNear() const;
   void setNear(float z_near);
   float getFar() const;
   void setFar(float z_far);
   void lookAt(const Vector& position, const Vector& target, const Vector& up = Vector(0, 0, 1));
   bool getPerspectiveMode() const;
   void setPerspectiveMode(bool mode);

private:
   float _fov = 1.0f;
   float _near = 1.0f;
   float _far = 1000.0f;
   bool _perspective_mode = true;
   ValTrack _fov_track;
};
