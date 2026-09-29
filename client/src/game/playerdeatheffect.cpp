// GLES3 port of client/src/game/playerdeatheffect.cpp. See project memory for the full
// GPGPU-particle-simulation background and deathflowfieldanimation.cpp's own header comment for
// the fullscreen-quad/glMapBuffer/client-state adaptations. This file's own adaptations:
//
// - MainDrawable::getInstance()->getRenderBuffer(1) doesn't exist in this port (no MainDrawable -
//   see project memory on the menu system's own equivalent fix) - PlayerDeathEffect now owns its
//   own dedicated FrameBuffer (_deferred_buffer, DepthTexture-flagged), lazily sized to the current
//   game viewport, matching MenuDrawable's own established "own your FBO instead of borrowing
//   MainDrawable's" pattern.
// - glGetFloatv(GL_PROJECTION_MATRIX, ...) becomes activeDevice->getProjectionMatrix() (this
//   engine's own established equivalent, used everywhere else already).
// - gl_ProjectionMatrix in deathparticles-vert.glsl needed an explicit u_projection uniform (the
//   original relied on the fixed-function matrix stack) - _points_projection, set once per render()
//   call via activeDevice->getProjectionMatrix() (same value the particles' world-space positions
//   were captured against).
// - deathinitparams-frag.glsl's "center" uniform is a vec2, but the original set it with a
//   3-component Vector (glUniform3fv -> GL_INVALID_OPERATION), so it always stayed (0, 0). It is
//   set to (0, 0) explicitly here to keep the original's flow field offsets.
// - glPushMatrix/glLoadIdentity/glPopMatrix -> activeDevice->push(Matrix())/pop() (identity world
//   transform - particle positions are already baked into world space at capture time, matching
//   this port's established BitmapFont::draw() convention for the same situation).
// - glEnable/glDisable(GL_TEXTURE_2D), glEnable(GL_POINT_SPRITE)/glEnable(
//   GL_VERTEX_PROGRAM_POINT_SIZE) and glColor4f() have no GLES3 equivalent and are dropped -
//   textures are always "enabled" once sampled by a shader, point sprites/vertex-shader point
//   size are always active, and there is no fixed-function color to reset.

#include "playerdeatheffect.h"
#include "deathflowfieldanimation.h"
#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "materials/material.h"
#include "nodes/node.h"
#include "nodes/mesh.h"
#include "render/geometry.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/matrix.h"
#include "image/image.h"

PlayerDeathEffect::PlayerDeathEffect()
{
   Image image;
   image.load("data/game/flowfield_pointsprite");
   _particle_texture_id = activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight());

   image.load("data/game/flowfield");
   _flow_field_texture_id = activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight(), 1);

   _points_shader = activeDevice->loadShader("deathparticles-vert.glsl", "deathparticles-frag.glsl");
   _points_texture = activeDevice->getParameterIndex("texturemap");
   _points_color_map = activeDevice->getParameterIndex("colormap");
   _points_size = activeDevice->getParameterIndex("particleSize");
   _points_projection = activeDevice->getParameterIndex("u_projection");

   _flow_shader = activeDevice->loadShader("deathflowfield-vert.glsl", "deathflowfield-frag.glsl");
   _flow_vertex_pos_texture = activeDevice->getParameterIndex("vertexPosTexture");
   _flow_vertex_col_texture = activeDevice->getParameterIndex("vertexColTexture");
   _flow_field_texture = activeDevice->getParameterIndex("flowfieldTexture");
   _flow_center = activeDevice->getParameterIndex("center");
   _flow_field_scale = activeDevice->getParameterIndex("fieldScale");
   _flow_time_delta = activeDevice->getParameterIndex("timeDelta");

   _flow_init_pos_shader = activeDevice->loadShader("deathinitpositions-vert.glsl", "deathinitpositions-frag.glsl");
   _flow_init_pos_depth = activeDevice->getParameterIndex("depthmap");
   _flow_init_pos_inv_proj = activeDevice->getParameterIndex("invProj");

   _flow_init_param_shader = activeDevice->loadShader("deathinitparams-vert.glsl", "deathinitparams-frag.glsl");
   _flow_init_param_depth = activeDevice->getParameterIndex("depthmap");
   _flow_init_param_inv_proj = activeDevice->getParameterIndex("invProj");
   _flow_init_param_center = activeDevice->getParameterIndex("center");
}

PlayerDeathEffect::~PlayerDeathEffect()
{
   clear();
}

void PlayerDeathEffect::clear()
{
   _flow_animations.clear();
}

void PlayerDeathEffect::add(Material* material)
{
   Vector min;
   Vector max;

   if (!material)
      return;

   const int width = activeDevice->getWidth();
   const int height = activeDevice->getHeight();

   if (!_deferred_buffer)
      _deferred_buffer = std::make_unique<FrameBuffer>(width, height, 0, FrameBuffer::DepthTexture);
   else if (_deferred_buffer->resolutionChanged(width, height))
      _deferred_buffer->setResolution(width, height);

   FrameBuffer::push(_deferred_buffer.get());

   // TODO: clear relevant area only!
   // transparent black (the original's global clear color): this is the particles' color map,
   // so background texels must stay invisible.
   static_cast<GLDevice*>(activeDevice)->clear(0.0f, 0.0f, 0.0f, 0.0f);

   Matrix proj = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix();
   Matrix inv_proj = proj.invert4x4();

   material->renderDiffuse();

   // get pivot in world space
   Vector center = material->getGeometry(0)->getTransform().translation();

   material->getBoundingRect(min, max, proj);

   auto animation = std::make_unique<DeathFlowFieldAnimation>();
   animation->setCenter(center);

   animation->initialize(_deferred_buffer.get(), min, max);

   activeDevice->setShader(_flow_init_pos_shader);
   activeDevice->bindSampler(_flow_init_pos_depth, 0);
   activeDevice->setParameter(_flow_init_pos_inv_proj, inv_proj);
   animation->initializePositions(_deferred_buffer->depthTexture(), min, max);
   activeDevice->setShader(0);

   activeDevice->setShader(_flow_init_param_shader);
   activeDevice->bindSampler(_flow_init_param_depth, 0);
   activeDevice->setParameter(_flow_init_param_inv_proj, inv_proj);
   // (0, 0) as in the original, see the header comment
   activeDevice->setParameter(_flow_init_param_center, Vector2(0.0f, 0.0f));
   animation->initializeParams(_deferred_buffer->depthTexture(), min, max);
   activeDevice->setShader(0);

   FrameBuffer::pop();

   _flow_animations.push_back(std::move(animation));
}

void PlayerDeathEffect::animate(float delta)
{
   FrameBuffer::push();

   activeDevice->setShader(_flow_shader);

   glDepthMask(false);
   glDisable(GL_DEPTH_TEST);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _flow_field_texture_id);
   activeDevice->bindSampler(_flow_field_texture, 0);

   glActiveTexture(GL_TEXTURE1);
   activeDevice->bindSampler(_flow_vertex_pos_texture, 1);

   glActiveTexture(GL_TEXTURE2);
   activeDevice->bindSampler(_flow_vertex_col_texture, 2);

   for (auto it = _flow_animations.begin(); it != _flow_animations.end();)
   {
      DeathFlowFieldAnimation* flow = it->get();
      if (flow->isElapsed())
      {
         it = _flow_animations.erase(it);
      }
      else
      {
         activeDevice->setParameter(_flow_center, flow->getCenter());
         activeDevice->setParameter(_flow_field_scale, flow->getScale());
         activeDevice->setParameter(_flow_time_delta, delta);

         flow->update(delta);
         ++it;
      }
   }

   glDepthMask(true);
   glEnable(GL_DEPTH_TEST);

   glActiveTexture(GL_TEXTURE0);

   activeDevice->setShader(0);

   FrameBuffer::pop();
}

void PlayerDeathEffect::render()
{
   // particle positions are already baked into world space (captured once at death time) - push
   // an identity world transform, matching BitmapFont::draw()'s established convention for the
   // same situation.
   activeDevice->setShader(_points_shader);
   activeDevice->setParameter(_points_projection, static_cast<GLDevice*>(activeDevice)->getProjectionMatrix());
   activeDevice->push(Matrix());

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_FALSE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _particle_texture_id);
   activeDevice->bindSampler(_points_texture, 0);

   glActiveTexture(GL_TEXTURE1);
   activeDevice->bindSampler(_points_color_map, 1);

   // the original always rendered into a screen-sized FrameBuffer; without one, scale by the
   // screen width so particle size keeps its 1920 px reference.
   float size_factor = static_cast<float>(activeDevice->getWidth()) / 1920.0f;
   FrameBuffer* fb = FrameBuffer::Instance();
   if (fb)
      size_factor = fb->getSizeFactor(1920.0f);

   for (const auto& flow : _flow_animations)
   {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, flow->getColorMap());
      activeDevice->setParameter(_points_size, flow->getPointSize() * size_factor);

      flow->draw();
   }

   glActiveTexture(GL_TEXTURE0);

   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);

   activeDevice->pop();
   activeDevice->setShader(0);
}
