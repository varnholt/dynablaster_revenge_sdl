#include "detonationmanager.h"
#include "detonation.h"
#include "framework/gldevice.h"
#include "math/vector.h"
#include "math/vector4.h"
#include "image/image.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>

namespace
{

float linear(float a, float b, float t)
{
   return a + (b-a)*t;
}

float fract( float x )
{
   return std::fmod(x, 1.0f);
}

float hash( float n )
{
   return fract(std::sin(n)*43758.5453f);
}

Vector vfloor( const Vector& x )
{
   Vector v;
   v.x= floorf(x.x);
   v.y= floorf(x.y);
   v.z= floorf(x.z);
   return v;
}

float mix(float x, float y, float a)
{
   return x + (y-x)*a;
}

Vector4 mix(const Vector4& x, const Vector4& y, float a)
{
   return x + (y-x)*a;
}

float frand(float min, float max)
{
   int x= rand() & 16384;
   return min + x * (max - min) / 16383.0f;
}

float noise( const Vector& x )
{
   Vector p = vfloor(x);

   Vector f = Vector( frand(0.0f, 1.0f), frand(0.0f, 1.0f), frand(0.0f, 1.0f) );

   float n = p.x + p.y*57.0f + 113.0f*p.z;

   float res =
      mix( mix( mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
      mix( hash(n+ 57.0f), hash(n+ 58.0f),f.x),f.y),
      mix(mix( hash(n+113.0f), hash(n+114.0f),f.x),
      mix( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);

   return res;
}

Vector4 gradient(float x)
{
   const Vector4 c0 = Vector4(0.1f, 0.1f, 0.1f, 0.0f);   // transparent
   const Vector4 c1 = Vector4(0.1f, 0.1f, 0.1f, 0.6f);   // grey
   const Vector4 c2 = Vector4(0.1f, 0.1f, 0.1f, 0.0f);   // black
   const Vector4 c3 = Vector4(1.0f, 0.2f, 0.0f, 0.6f);   // red
   const Vector4 c4 = Vector4(1.2f, 0.5f, 0.3f, 1.0f);   // yellow
   const Vector4 c5 = Vector4(1.0f, 0.7f, 0.0f, 1.0f);   // yellow

   float t = fract(x*5.0f);

   Vector4 c;
   if (x < 0.0f)
      return c0;
   else if (x < 0.2f)
      c =  mix(c0, c1, t);
   else if (x < 0.4f)
      c = mix(c1, c2, t);
   else if (x < 0.6f)
      c = mix(c2, c3, t);
   else if (x < 0.8f)
      c = mix(c3, c4, t);
   else if (x < 1.0f)
      c = mix(c4, c5, t);
   else if (x >= 1.0f)
      return c5;

   return c;
}

}  // namespace

DetonationManager::DetonationManager()
{
}

DetonationManager::~DetonationManager()
{
   clear();
}

void DetonationManager::clear()
{
   _detonations.clear();
}

void DetonationManager::init()
{
   GLDevice* dev = static_cast<GLDevice*>(activeDevice);

   // gradient palette - GLES3 has no GL_TEXTURE_1D, uploaded as a 2D texture with height 1
   // instead (sampled at v=0.5 in flame-frag.glsl).
   Image palette("detonationpalette");

   glGenTextures(1, &_gradient_map);
   glBindTexture(GL_TEXTURE_2D, _gradient_map);
   glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      palette.getWidth(),
      1,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      palette.getData()
   );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   // procedural noise volume - GLES3 has native GL_TEXTURE_3D support, ported as-is except the
   // internal format: GL_ALPHA isn't part of GLES3's texImage3D format table, R8 is the modern
   // single-channel equivalent (read back via .r instead of .a in the shader).
   const int size = 32;
   std::vector<uint8_t> noise_map(static_cast<size_t>(size) * size * size);
   for (int z = 0; z < size; z++)
   {
      const float scale = 1.0f / size;
      for (int y = 0; y < size; y++)
      {
         for (int x = 0; x < size; x++)
         {
            Vector p(x*scale, y*scale, z*scale);
            float n1 = noise(p);
            noise_map[static_cast<size_t>((z*size+y)*size+x)] = static_cast<uint8_t>(n1*127.0f+128.0f);
         }
      }
   }

   glGenTextures(1, &_noise_map);
   glBindTexture(GL_TEXTURE_3D, _noise_map);
   glTexImage3D(
      GL_TEXTURE_3D,
      0,
      GL_R8,
      size,
      size,
      size,
      0,
      GL_RED,
      GL_UNSIGNED_BYTE,
      noise_map.data()
   );
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glBindTexture(GL_TEXTURE_3D, 0);

   // load shader
   _shader = dev->loadShader("flame-vert.glsl", "flame-frag.glsl");

   _param_time = dev->getParameterIndex("time");
   _param_cam_pos = dev->getParameterIndex("campos");
   _param_noise_map = dev->getParameterIndex("noisemap");
   _param_gradient_map = dev->getParameterIndex("gradientmap");
   _param_top = dev->getParameterIndex("top");
   _param_bottom = dev->getParameterIndex("bottom");
   _param_left = dev->getParameterIndex("left");
   _param_right = dev->getParameterIndex("right");
   _param_bound_min = dev->getParameterIndex("boundmin");
   _param_bound_max = dev->getParameterIndex("boundmax");
}

void DetonationManager::addDetonation(int x, int y, int top, int bottom, int left, int right)
{
   auto det = std::make_unique<Detonation>(x, y + 1, left, right, top, bottom);
   det->setStartTime(_time);
   _detonations.push_back(std::move(det));
}

void DetonationManager::update(float time)
{
   _time= time;
   std::erase_if(_detonations, [time](const std::unique_ptr<Detonation>& detonation) { return detonation->elapsed(time) > 2.0f; });
}

void DetonationManager::drawBox(float x, float y, float z, float left, float right, float bottom, float top, int sides)
{
   static const uint8_t tris[5*3*2]= {
      2,6,7,  3,2,7, // back   +y
      0,1,4,  1,5,4, // front  -y
      0,4,6,  0,6,2, // left   -x
      1,3,5,  3,7,5, // right  +x
      4,5,6,  6,5,7  // top    +z
   };

   sides|=16;

   Vector boxmin(left, bottom, -0.5f);
   Vector boxmax(right, top, 0.5f);
   if ((sides&1)==0) boxmax.y++;
   if ((sides&2)==0) boxmin.y--;
   if ((sides&4)==0) boxmin.x--;
   if ((sides&8)==0) boxmax.x++;

   activeDevice->setParameter(_param_bound_min, boxmin);
   activeDevice->setParameter(_param_bound_max, boxmax);

   // position (3) + normal (3) per vertex, built on the CPU and uploaded as a dynamic attribute-
   // array buffer instead of glBegin(GL_TRIANGLES)/glVertex3f/glNormal3f immediate mode.
   std::vector<float> vertices;
   vertices.reserve(30 * 6);

   int side_bits = sides;
   for (int i=0; i<5; i++)
   {
      if (side_bits&1)
      {
         for (int tri=0;tri<6;tri++)
         {
            const int index= tris[i*6+tri];
            Vector v(static_cast<float>(index&1), static_cast<float>(index>>1&1), static_cast<float>(index>>2&1));

            vertices.push_back(v.x*x);
            vertices.push_back(v.y*y);
            vertices.push_back(v.z*z);

            vertices.push_back(linear(left, right, v.x));
            vertices.push_back(linear(bottom, top, v.y));
            vertices.push_back(v.z-0.5f);
         }
      }
      side_bits>>=1;
   }

   if (vertices.empty())
      return;

   const int byte_size = static_cast<int>(vertices.size() * sizeof(float));

   if (_box_vertex_buffer == 0)
      _box_vertex_buffer = activeDevice->createVertexBuffer(byte_size, true);
   else
      activeDevice->allocateVertexBuffer(_box_vertex_buffer, byte_size, true);

   void* dst = activeDevice->lockVertexBuffer(_box_vertex_buffer, byte_size);
   std::memcpy(dst, vertices.data(), static_cast<size_t>(byte_size));
   activeDevice->unlockVertexBuffer(_box_vertex_buffer);

   glBindBuffer(GL_ARRAY_BUFFER, _box_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (GLvoid*)0);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (GLvoid*)(sizeof(float)*3));

   glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 6));

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}

void DetonationManager::drawExplosion(Detonation *det, float time)
{
   GLDevice* dev= static_cast<GLDevice*>(activeDevice);

   time= det->elapsed(time);
   dev->setParameter(_param_time, time);

   float x= static_cast<float>(det->getX());
   float y= static_cast<float>(det->getY());

   float top= static_cast<float>(det->getUp());
   float bottom= static_cast<float>(det->getDown());
   float left= static_cast<float>(det->getLeft());
   float right= static_cast<float>(det->getRight());

   dev->setParameter(_param_top, top);
   dev->setParameter(_param_bottom, bottom);
   dev->setParameter(_param_left, left);
   dev->setParameter(_param_right, right);

   // top
   activeDevice->push(Matrix::position(x,-y+1,0.0f));
   drawBox(1.0f, top, 1.0f, -0.5f, 0.5f, 0.5f, top+0.5f, 1|4|8); // top !bottom left right
   activeDevice->pop();

   // center
   activeDevice->push(Matrix::position(x,-y,0.0f));
   drawBox(1.0f, 1.0f, 1.0f, -0.5f, 0.5f, -0.5f,0.5f, 0); // !top !bottom !left !right
   activeDevice->pop();

   // left
   activeDevice->push(Matrix::position(x-left,-y,0.0f));
   drawBox(left, 1.0f, 1.0f, -left-0.5f, -0.5f, -0.5f,0.5f, 1|2|4); // top bottom left !right
   activeDevice->pop();

   // right
   activeDevice->push(Matrix::position(x+1,-y,0.0f));
   drawBox(right, 1.0f, 1.0f, 0.5f, right+0.5f, -0.5f,0.5f, 1|2|8); // top bottom !left right
   activeDevice->pop();

   // bottom
   activeDevice->push(Matrix::position(x,-y-bottom,0.0f));
   drawBox(1.0f, bottom, 1.0f, -0.5f, 0.5f, -bottom-0.5f, -0.5f, 2|4|8); // !top bottom left right
   activeDevice->pop();
}

void DetonationManager::render()
{
   if (_detonations.empty())
      return;

   GLDevice* dev= static_cast<GLDevice*>(activeDevice);

   // replaces the legacy glGetFloatv(GL_PROJECTION_MATRIX, ...) readback - see
   // GLDevice::getProjectionMatrix()'s own doc comment for why this is the right replacement
   // (this engine's "projection" matrix already carries view*projection combined).
   Matrix proj_mat = dev->getProjectionMatrix();
   proj_mat = proj_mat.invert();
   Vector cam_pos = proj_mat.translation();

   glEnable(GL_BLEND);

   dev->setShader(_shader);
   dev->setParameter(_param_cam_pos, cam_pos);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, _noise_map);
   dev->bindSampler(_param_noise_map, 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _gradient_map);
   dev->bindSampler(_param_gradient_map, 1);

   for (const auto& det : _detonations)
   {
      drawExplosion(det.get(), _time);
   }

   dev->setShader(0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, 0);

   glDisable(GL_BLEND);
}
