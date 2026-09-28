#include "playerinvincibleeffect.h"
#include "playerinvincibleinstance.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "framework/globaltime.h"
#include "materials/material.h"
#include "math/matrix.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "render/geometry.h"
#include "render/texturepool.h"

#include <cmath>

namespace
{
// the blur passes key off alpha, so the offscreen targets must clear to alpha 0 like the
// old device's global clear color; this port's default clear color is opaque.
void clearTransparent()
{
   static_cast<GLDevice*>(activeDevice)->clear(0.0f, 0.0f, 0.0f, 0.0f);
}

// draws a quad (as 2 triangles) with an explicit position component count (2 for NDC-space
// blur-pass quads, 3 for the world-space displace-pass quad) plus an optional uv pair.
void drawQuad(const float* verts, int pos_components, int uv_components)
{
   const int floats_per_vertex = pos_components + uv_components;
   const int order[6] = {0, 1, 2, 0, 2, 3};
   float buffer[6 * 5];

   for (int i = 0; i < 6; i++)
   {
      const float* src = verts + order[i] * floats_per_vertex;
      float* dst = buffer + i * floats_per_vertex;
      for (int c = 0; c < floats_per_vertex; c++)
      {
         dst[c] = src[c];
      }
   }

   static unsigned int quad_vertex_buffer = 0;
   if (quad_vertex_buffer == 0)
   {
      glGenBuffers(1, &quad_vertex_buffer);
   }

   glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * floats_per_vertex, buffer, GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, pos_components, GL_FLOAT, GL_FALSE, sizeof(float) * floats_per_vertex, (GLvoid*)0);

   if (uv_components > 0)
   {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, uv_components, GL_FLOAT, GL_FALSE, sizeof(float) * floats_per_vertex, (GLvoid*)(sizeof(float) * pos_components));
   }

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   if (uv_components > 0)
   {
      glDisableVertexAttribArray(1);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// perspective-corrected back-projection of a 2d screen rect into 3d world space, using a
// reference triangle placed in front of the player as the known world<->screen correspondence.
void backProject(Vector* dst, const Vector& min2d, const Vector& max2d, const Matrix& obj_mat, const Matrix& proj_mat)
{
   struct Vertex
   {
      float x, y, z;
      float u, v, w;
   };

   Vector pos = obj_mat.translation();

   Vector world_tri[3];
   world_tri[0] = Vector(-0.6f, -0.5f, 2.0f) + pos;
   world_tri[1] = Vector(-0.6f, -0.5f, 0.0f) + pos;
   world_tri[2] = Vector(0.6f, -0.5f, 0.0f) + pos;

   Vertex vtx[3];
   const Matrix& mat = proj_mat;
   for (int i = 0; i < 3; i++)
   {
      const Vector& v = world_tri[i];
      float x = mat.xx * v.x + mat.xy * v.y + mat.xz * v.z + mat.xw;
      float y = mat.yx * v.x + mat.yy * v.y + mat.yz * v.z + mat.yw;
      float w = mat.wx * v.x + mat.wy * v.y + mat.wz * v.z + mat.ww;
      float t = 1.0f / w;
      vtx[i].x = x * t;
      vtx[i].y = y * t;
      vtx[i].z = t;
      vtx[i].u = v.x * t;
      vtx[i].v = v.y * t;
      vtx[i].w = v.z * t;
   }

   float delta1 = vtx[1].y - vtx[2].y;
   float delta2 = vtx[0].y - vtx[2].y;
   float denom = (vtx[0].x - vtx[2].x) * delta1 - (vtx[1].x - vtx[2].x) * delta2;

   denom = 1.0f / denom;
   delta1 *= denom;
   delta2 *= denom;

   float deltau = (vtx[0].u - vtx[2].u) * delta1 - (vtx[1].u - vtx[2].u) * delta2;
   float deltav = (vtx[0].v - vtx[2].v) * delta1 - (vtx[1].v - vtx[2].v) * delta2;
   float deltaw = (vtx[0].w - vtx[2].w) * delta1 - (vtx[1].w - vtx[2].w) * delta2;
   float deltaz = (vtx[0].z - vtx[2].z) * delta1 - (vtx[1].z - vtx[2].z) * delta2;

   int min_vtx = 0;
   for (int n = 1; n < 3; n++)
   {
      if (vtx[n].y < vtx[min_vtx].y)
      {
         min_vtx = n;
      }
   }

   Vertex* v1 = &vtx[min_vtx];
   min_vtx++;
   if (min_vtx > 2)
   {
      min_vtx = 0;
   }
   Vertex* v2 = &vtx[min_vtx];
   float inv_height = 1.0f / (v2->y - v1->y);
   float left_dx = (v2->x - v1->x) * inv_height;
   float left_dz = (v2->z - v1->z) * inv_height;
   float left_du = (v2->u - v1->u) * inv_height;
   float left_dv = (v2->v - v1->v) * inv_height;
   float left_dw = (v2->w - v1->w) * inv_height;

   Vector rect2d[4];
   rect2d[0] = Vector(min2d.x, min2d.y);
   rect2d[1] = Vector(max2d.x, min2d.y);
   rect2d[2] = Vector(max2d.x, max2d.y);
   rect2d[3] = Vector(min2d.x, max2d.y);

   for (int i = 0; i < 4; i++)
   {
      float dx = rect2d[i].x - v1->x;
      float dy = rect2d[i].y - v1->y;

      float u = v1->u + dy * left_du - dy * left_dx * deltau + dx * deltau;
      float v = v1->v + dy * left_dv - dy * left_dx * deltav + dx * deltav;
      float w = v1->w + dy * left_dw - dy * left_dx * deltaw + dx * deltaw;
      float z = v1->z + dy * left_dz - dy * left_dx * deltaz + dx * deltaz;

      float t = 1.0f / z;

      dst[i].x = u * t;
      dst[i].y = v * t;
      dst[i].z = w * t;
   }
}
}  // namespace

PlayerInvincibleEffect::PlayerInvincibleEffect()
   : _radius(0.0f),
     _blur_h_shader(0),
     _blur_h_texture(-1),
     _blur_h_texel_offset(-1),
     _blur_h_radius(-1),
     _blur_h_kernel(-1),
     _blur_v_shader(0),
     _blur_v_texture(-1),
     _blur_v_texel_offset(-1),
     _blur_v_radius(-1),
     _blur_v_kernel(-1),
     _displace_shader(0),
     _displace_texture1(-1),
     _displace_texture2(-1),
     _displace_texel_offset(-1),
     _displace_offset(-1),
     _displace_source_rect(-1),
     _displace_fade(-1),
     _display_center(-1),
     _scratch_buffer(nullptr)
{
   _displacement_texture = TexturePool::Instance()->getTexture("data/game/displace");

   setRadius(10.0f);

   _blur_h_shader = activeDevice->loadShader("playerinvincibleblurh-vert.glsl", "playerinvincibleblurh-frag.glsl");
   _blur_h_texture = activeDevice->getParameterIndex("texturemap");
   _blur_h_texel_offset = activeDevice->getParameterIndex("texelOffset");
   _blur_h_radius = activeDevice->getParameterIndex("radius");
   _blur_h_kernel = activeDevice->getParameterIndex("kernel");

   _blur_v_shader = activeDevice->loadShader("playerinvincibleblurv-vert.glsl", "playerinvincibleblurv-frag.glsl");
   _blur_v_texture = activeDevice->getParameterIndex("texturemap");
   _blur_v_texel_offset = activeDevice->getParameterIndex("texelOffset");
   _blur_v_radius = activeDevice->getParameterIndex("radius");
   _blur_v_kernel = activeDevice->getParameterIndex("kernel");

   _displace_shader = activeDevice->loadShader("playerinvincibledisplace-vert.glsl", "playerinvincibledisplace-frag.glsl");
   _displace_texture1 = activeDevice->getParameterIndex("texturemap");
   _displace_texture2 = activeDevice->getParameterIndex("displace");
   _displace_texel_offset = activeDevice->getParameterIndex("texelOffset");
   _displace_offset = activeDevice->getParameterIndex("displaceOffset");
   _displace_source_rect = activeDevice->getParameterIndex("sourceRect");
   _displace_fade = activeDevice->getParameterIndex("fade");
   _display_center = activeDevice->getParameterIndex("center");
}

PlayerInvincibleEffect::~PlayerInvincibleEffect()
{
   clear();
   delete _scratch_buffer;
}

void PlayerInvincibleEffect::setRadius(float radius)
{
   if (radius > 30.0f)
   {
      radius = 30.0f;
   }

   _radius = radius;
   int size = static_cast<int>(std::ceil(_radius));
   const double scale = -4.0 / (_radius * _radius);
   float sum = 0.0f;
   for (int i = 0; i <= size; i++)
   {
      float f = static_cast<float>(std::pow(2.718281828459045, i * i * scale));
      sum += f;
      _kernel[i] = f;
   }
   for (int i = size + 1; i < 32; i++)
   {
      _kernel[i] = 0.0f;
   }

   float t = 1.1f / (sum * 2.0f - 1.0f);
   for (int i = 0; i <= size; i++)
   {
      _kernel[i] *= t;
   }
}

void PlayerInvincibleEffect::clear()
{
   _players.clear();
}

void PlayerInvincibleEffect::add(Material* material)
{
   if (!material)
   {
      return;
   }

   auto it = _players.find(material);
   if (it == _players.end())
   {
      auto player = std::make_unique<PlayerInvincibleInstance>();
      player->setMaterial(material);
      _players[material] = std::move(player);
   }
   else
   {
      it->second->setRemove(false);
   }
}

void PlayerInvincibleEffect::remove(Material* player_material)
{
   auto it = _players.find(player_material);
   if (it != _players.end())
   {
      it->second->remove();
   }
}

void PlayerInvincibleEffect::setMaterialFade(PlayerInvincibleInstance* player, float fade)
{
   Material* mat = player->getMaterial();
   if (!mat)
   {
      return;
   }

   Geometry* geo = mat->getGeometry(0);
   if (!geo)
   {
      return;
   }

   Mesh* mesh = static_cast<Mesh*>(geo->getParent());
   if (mesh)
   {
      mesh->setRenderParameter(2, fade);
   }
}

void PlayerInvincibleEffect::animate(float dt)
{
   for (auto it = _players.begin(); it != _players.end();)
   {
      PlayerInvincibleInstance* player = it->second.get();

      if (!player->update(dt))
      {
         setMaterialFade(player, 0.0f);
         it = _players.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

void PlayerInvincibleEffect::blurPlayers(FrameBuffer* temp, const Matrix& proj)
{
   FrameBuffer::push();

   for (auto& [material, player] : _players)
   {
      Material* mat = player->getMaterial();
      if (!mat)
      {
         continue;
      }

      Vector min2d(0.0f, 0.0f, 0.0f);
      Vector max2d(0.0f, 0.0f, 0.0f);

      mat->getBoundingRect(min2d, max2d, proj);

      float bx = _radius * 2.0f / temp->width();
      float by = _radius * 2.0f / temp->height();
      min2d.x -= bx;
      min2d.y -= by;
      max2d.x += bx;
      max2d.y += by;

      player->setRect(min2d, max2d);
      player->setCenter(mat->getCenter2d(proj));

      min2d.x = (min2d.x + 1.0f) * 0.5f;
      min2d.y = (min2d.y + 1.0f) * 0.5f;
      max2d.x = (max2d.x + 1.0f) * 0.5f;
      max2d.y = (max2d.y + 1.0f) * 0.5f;

      temp->bind();
      clearTransparent();
      setMaterialFade(player.get(), player->getFade());
      mat->renderDiffuse();
      temp->unbind();

      // horizontal blur - temp framebuffer into the player's own ping-pong texture 0
      player->bind(0);
      clearTransparent();
      activeDevice->setShader(_blur_h_shader);
      glBindTexture(GL_TEXTURE_2D, temp->texture());
      activeDevice->bindSampler(_blur_h_texture, 0);
      activeDevice->setParameter(_blur_h_texel_offset, Vector2(1.0f / temp->width()));
      activeDevice->setParameter(_blur_h_radius, _radius);
      activeDevice->setParameter(_blur_h_kernel, _kernel, 32);

      float x = (max2d.x - min2d.x) * 2.0f * temp->width() / player->width();
      float y = (max2d.y - min2d.y) * 2.0f * temp->height() / player->height();

      const float quad_h[4 * 4] = {
         -1.0f, -1.0f, min2d.x, min2d.y, -1.0f + x, -1.0f, max2d.x, min2d.y,
         -1.0f + x, -1.0f + y, max2d.x, max2d.y, -1.0f, -1.0f + y, min2d.x, max2d.y,
      };
      drawQuad(quad_h, 2, 2);

      player->unbind();

      // vertical blur - texture 0 into texture 1
      player->bind(1);
      clearTransparent();
      activeDevice->setShader(_blur_v_shader);
      glBindTexture(GL_TEXTURE_2D, player->texture(0));
      activeDevice->setParameter(_blur_v_texel_offset, Vector2(0.0f, 1.0f / player->height()));
      activeDevice->setParameter(_blur_v_radius, _radius);
      activeDevice->setParameter(_blur_v_kernel, _kernel, 32);

      const float quad_v[4 * 4] = {
         -1.0f, -1.0f, 0.0f, 0.0f, -1.0f + x, -1.0f, x * 0.5f, 0.0f,
         -1.0f + x, -1.0f + y, x * 0.5f, y * 0.5f, -1.0f, -1.0f + y, 0.0f, y * 0.5f,
      };
      drawQuad(quad_v, 2, 2);

      player->unbind();

      activeDevice->setShader(0);
   }

   FrameBuffer::pop();
}

void PlayerInvincibleEffect::render()
{
   if (_players.empty())
   {
      return;
   }

   Matrix proj = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix();

   float time = GlobalTime::Instance()->getTime() * 0.1f;

   const int width = activeDevice->getWidth();
   const int height = activeDevice->getHeight();

   if (!_scratch_buffer)
   {
      _scratch_buffer = new FrameBuffer(width, height, 0, FrameBuffer::NoDepthBuffer);
   }
   else if (_scratch_buffer->resolutionChanged(width, height))
   {
      _scratch_buffer->setResolution(width, height);
   }

   blurPlayers(_scratch_buffer, proj);

   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(_displace_shader);

   // identity world transform (old glLoadIdentity), uploads u_modelViewProjection = projection
   activeDevice->push(Matrix());

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _displacement_texture);
   activeDevice->bindSampler(_displace_texture2, 1);

   glActiveTexture(GL_TEXTURE0);
   activeDevice->bindSampler(_displace_texture1, 0);

   for (auto& [material, player] : _players)
   {
      Material* mat = player->getMaterial();
      if (!mat)
      {
         continue;
      }

      Geometry* geo = mat->getGeometry(0);
      if (!geo)
      {
         continue;
      }

      const Matrix& tm = geo->getTransform();

      Vector min2d = player->min2d();
      Vector max2d = player->max2d();

      activeDevice->setParameter(_displace_texel_offset, Vector2(_scratch_buffer->width() / 1920.0f, _scratch_buffer->height() / 1080.0f));

      activeDevice->setParameter(
         _displace_source_rect,
         Vector4(min2d.x, min2d.y, 0.5f * _scratch_buffer->width() / player->width(), 0.5f * _scratch_buffer->height() / player->height())
      );

      activeDevice->setParameter(
         _displace_offset,
         Vector4(
            std::sin(time * 2.69 * 0.025) * 0.5f * 1920.0f / _scratch_buffer->width(),
            -(time * 0.5f + std::sin(time * 2.81 * 0.025) * 0.5f) * 1920.0f / _scratch_buffer->width(),
            -std::cos(time * 3.11 * 0.025) * 0.5f * 1920.0f / _scratch_buffer->width(),
            -(time * 0.5f - std::cos(time * 3.59 * 0.025) * 0.5f) * 1920.0f / _scratch_buffer->width()
         )
      );

      activeDevice->setParameter(_displace_fade, player->getFade());
      activeDevice->setParameter(_display_center, player->getCenter());

      Vector pos[4];
      backProject(pos, min2d, max2d, tm, proj);

      glBindTexture(GL_TEXTURE_2D, player->texture(1));

      const float quad[4 * 3] = {
         pos[0].x, pos[0].y, pos[0].z, pos[1].x, pos[1].y, pos[1].z, pos[2].x, pos[2].y, pos[2].z, pos[3].x, pos[3].y, pos[3].z,
      };
      drawQuad(quad, 3, 0);
   }

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_TRUE);

   activeDevice->pop();
   activeDevice->setShader(0);
}
