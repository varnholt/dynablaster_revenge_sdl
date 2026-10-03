#include "bombfuzegeometryvbo.h"

// engine
#include "gldevice.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "render/geometry.h"
#include "render/texturepool.h"

BombFuzeGeometryVbo::BombFuzeGeometryVbo(const Geometry& geometry) : GeometryVbo(geometry)
{
}

void BombFuzeGeometryVbo::initialize()
{
   GeometryVbo::initialize();

   _texture = TexturePool::Instance().getTexture("fuze");

   _shader = activeDevice().loadShader("socketlight-vert.glsl", "socketlight-frag.glsl");
   _fresnel = activeDevice().getParameterIndex("fresnelFactor");
   _color_param = activeDevice().getParameterIndex("u_color");
}

void BombFuzeGeometryVbo::initGlParameters()
{
   activeDevice().setShader(_shader);
   activeDevice().setParameter(_fresnel, Vector2(1.02f, 3.0f));
}

void BombFuzeGeometryVbo::cleanupGlParameter()
{
   activeDevice().setShader(0);
}

void BombFuzeGeometryVbo::draw(const Vector4& color)
{
   initGlParameters();

   activeDevice().setParameter(_color_param, color);

   Matrix rotation = Matrix::rotateX(-1.0f);     // rotate the north pole towards the viewer
   rotation = rotation * Matrix::rotateY(1.0f);  // make earth rotate / instead of |

   glBindTexture(GL_TEXTURE_2D, _texture.getTexture());

   activeDevice().push(_geometry.getTransform() * rotation);
   drawGeometry();
   activeDevice().pop();

   cleanupGlParameter();
}
