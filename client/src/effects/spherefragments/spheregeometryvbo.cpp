#include "spheregeometryvbo.h"

// engine
#include "framework/globaltime.h"
#include "gldevice.h"
#include "math/vector.h"
#include "math/vector4.h"
#include "render/geometry.h"
#include "render/texturepool.h"

#include <numbers>

namespace
{
constexpr float DEGREES_TO_RADIANS = std::numbers::pi_v<float> / 180.0f;
}  // namespace

SphereGeometryVbo::SphereGeometryVbo(const Geometry& geometry) : GeometryVbo(geometry)
{
}

void SphereGeometryVbo::initialize()
{
   GeometryVbo::initialize();

   _texture = TexturePool::Instance().getTexture("bomb");

   // the spherefragments shader directory takes precedence over the generic one of the same name
   _shader = activeDevice().loadShader("simplelight-vert.glsl", "simplelight-frag.glsl");
   _color_param = activeDevice().getParameterIndex("u_color");
}

void SphereGeometryVbo::initGlParameters()
{
   activeDevice().setShader(_shader);
}

void SphereGeometryVbo::cleanupGlParameter()
{
   activeDevice().setShader(0);
}

void SphereGeometryVbo::draw(const Vector4& color)
{
   initGlParameters();

   activeDevice().setParameter(_color_param, color);

   Matrix rotation = Matrix::rotateY(GlobalTime::Instance().getTime() * 10.0f * DEGREES_TO_RADIANS);
   rotation = rotation * Matrix::rotateX(23.5f * DEGREES_TO_RADIANS);

   glBindTexture(GL_TEXTURE_2D, _texture.getTexture());

   activeDevice().push(_geometry.getTransform() * rotation);
   drawGeometry();
   activeDevice().pop();

   cleanupGlParameter();
}
