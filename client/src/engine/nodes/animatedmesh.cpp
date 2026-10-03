#include "animatedmesh.h"
#include "render/geometry.h"

AnimatedMesh::AnimatedMesh()
{
   setUserTransformable(true);
}

int32_t AnimatedMesh::getFrameCount() const
{
   return getPartCount();
}

Geometry& AnimatedMesh::getFrame(int32_t frame)
{
   return getPart(frame);
}
