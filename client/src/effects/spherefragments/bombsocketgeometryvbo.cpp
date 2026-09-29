#include "bombsocketgeometryvbo.h"

// engine
#include "framework/globaltime.h"
#include "gldevice.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "render/geometry.h"
#include "render/texturepool.h"

BombSocketGeometryVbo::BombSocketGeometryVbo(Geometry* geometry) : GeometryVbo(geometry)
{
}

void BombSocketGeometryVbo::initialize()
{
   GeometryVbo::initialize();

   _texture = TexturePool::Instance()->getTexture("metal_fire");

   _shader = activeDevice->loadShader("socketlight-vert.glsl", "socketlight-frag.glsl");
   _fresnel = activeDevice->getParameterIndex("fresnelFactor");
   _color_param = activeDevice->getParameterIndex("u_color");
}

void BombSocketGeometryVbo::initGlParameters()
{
   activeDevice->setShader(_shader);
   activeDevice->setParameter(_fresnel, Vector2(1.3f, 8.0f));
}

void BombSocketGeometryVbo::cleanupGlParameter()
{
   activeDevice->setShader(0);
}

void BombSocketGeometryVbo::draw(const Vector4& color)
{
   initGlParameters();

   activeDevice->setParameter(_color_param, color);

   const float time = GlobalTime::Instance()->getTime();
   Matrix rotation = Matrix::rotateY(time * 0.1745f);  // rotate the earth around its own y axis
   rotation = rotation * Matrix::rotateX(-1.0f);       // rotate the north pole towards the viewer
   rotation = rotation * Matrix::rotateY(1.0f);        // make earth rotate / instead of |

   glBindTexture(GL_TEXTURE_2D, _texture.getTexture());

   activeDevice->push(_geometry->getTransform() * rotation);
   drawGeometry();
   activeDevice->pop();

   cleanupGlParameter();
}
