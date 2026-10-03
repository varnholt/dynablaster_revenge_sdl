#include "playerinfectedeffect.h"
#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "image/image.h"
#include "infectedflowfieldanimation.h"
#include "materials/material.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "render/geometry.h"

PlayerInfectedEffect::PlayerInfectedEffect()
{
   Image image;
   image.load("data/game/flowfield_pointsprite");
   _particle_texture_id = activeDevice().createTexture(image.getData(), image.getWidth(), image.getHeight());

   image.load("data/game/flowfield");
   _flow_field_texture_id = activeDevice().createTexture(image.getData(), image.getWidth(), image.getHeight(), 1);

   _points_shader = activeDevice().loadShader("infectedparticles-vert.glsl", "infectedparticles-frag.glsl");
   _points_texture = activeDevice().getParameterIndex("texturemap");
   _points_size = activeDevice().getParameterIndex("particleSize");
   _points_projection = activeDevice().getParameterIndex("u_projection");

   _flow_update_pos_shader = activeDevice().loadShader("infectedflowfield-vert.glsl", "infectedflowfield-frag.glsl");
   _flow_depth_texture = activeDevice().getParameterIndex("depthTexture");
   _flow_vertex_pos_texture = activeDevice().getParameterIndex("vertexPosTexture");
   _flow_vertex_param_texture = activeDevice().getParameterIndex("vertexParamTexture");
   _flow_field_texture = activeDevice().getParameterIndex("flowfieldTexture");
   _flow_center = activeDevice().getParameterIndex("center");
   _flow_field_scale = activeDevice().getParameterIndex("fieldScale");
   _flow_time_delta = activeDevice().getParameterIndex("timeDelta");
   _flow_src_rect = activeDevice().getParameterIndex("srcRect");
   _flow_inv_proj = activeDevice().getParameterIndex("invProj");
   _flow_stop = activeDevice().getParameterIndex("stop");

   _flow_init_pos_shader = activeDevice().loadShader("infectedinitpositions-vert.glsl", "infectedinitpositions-frag.glsl");
   _flow_init_pos_depth = activeDevice().getParameterIndex("depthmap");
   _flow_init_pos_inv_proj = activeDevice().getParameterIndex("invProj");
   _flow_init_src_rect = activeDevice().getParameterIndex("srcRect");

   _flow_init_param_shader = activeDevice().loadShader("infectedinitparams-vert.glsl", "infectedinitparams-frag.glsl");
   _flow_init_param_depth = activeDevice().getParameterIndex("depthmap");
   _flow_init_param_inv_proj = activeDevice().getParameterIndex("invProj");
   _flow_init_param_center = activeDevice().getParameterIndex("center");

   _flow_update_col_shader = activeDevice().loadShader("infectedupdatecolors-vert.glsl", "infectedupdatecolors-frag.glsl");
   _flow_update_col_color_map = activeDevice().getParameterIndex("colormap");
   _flow_update_col_position_map = activeDevice().getParameterIndex("positionmap");
   _flow_update_col_proj = activeDevice().getParameterIndex("proj");
   _flow_update_col_stop = activeDevice().getParameterIndex("stop");
}

PlayerInfectedEffect::~PlayerInfectedEffect()
{
   clear();
}

void PlayerInfectedEffect::clear()
{
   _flow_animations.clear();
}

void PlayerInfectedEffect::add(Material* material)
{
   if (!material)
   {
      return;
   }

   Flow flow;
   flow.animation = std::make_unique<InfectedFlowFieldAnimation>();
   flow.material = material;
   _flow_animations.push_back(std::move(flow));
}

void PlayerInfectedEffect::remove(Material* material)
{
   for (auto& flow : _flow_animations)
   {
      if (flow.material == material)
      {
         flow.animation->stop();
      }
   }
}

void PlayerInfectedEffect::animate(float delta)
{
   _delta = delta;

   std::erase_if(_flow_animations, [](const Flow& flow) { return flow.animation->isElapsed(); });
}

void PlayerInfectedEffect::render()
{
   Matrix proj = static_cast<GLDevice&>(activeDevice()).getProjectionMatrix();
   Matrix inv_proj = proj.invert4x4();

   const int width = activeDevice().getWidth();
   const int height = activeDevice().getHeight();

   if (!_deferred_buffer)
   {
      _deferred_buffer = std::make_unique<FrameBuffer>(width, height, 0, FrameBuffer::DepthTexture);
   }
   else if (_deferred_buffer->resolutionChanged(width, height))
   {
      _deferred_buffer->setResolution(width, height);
   }

   FrameBuffer::push();

   for (auto& flow : _flow_animations)
   {
      if (!flow.material)
      {
         continue;
      }

      // re-capture the infected player's current silhouette every frame - unlike PlayerDeathEffect
      // (a frozen one-shot capture), an infected player is still alive and moving. Depth writes
      // must still be on here: the particle passes below unproject from this depth texture.
      // Clear to transparent black (the original's global clear color): respawning particles
      // that sample the background must get alpha 0 so the color pass leaves them alone.
      FrameBuffer::push(*_deferred_buffer);
      static_cast<GLDevice&>(activeDevice()).clear(0.0f, 0.0f, 0.0f, 0.0f);

      Vector min;
      Vector max;
      flow.material->getBoundingRect(min, max, proj);
      Vector center = flow.material->getGeometry(0)->get().getTransform().translation();
      flow.animation->setCenter(center);

      flow.material->renderDiffuse();

      FrameBuffer::pop();

      glDepthMask(GL_FALSE);
      glDisable(GL_DEPTH_TEST);

      if (!flow.animation->initialized())
      {
         flow.animation->initialize();
         flow.animation->setInitialized(true);

         activeDevice().setShader(_flow_init_pos_shader);
         activeDevice().bindSampler(_flow_init_pos_depth, 0);
         activeDevice().setParameter(_flow_init_pos_inv_proj, inv_proj);
         activeDevice().setParameter(_flow_init_src_rect, Vector4(min.x, min.y, max.x, max.y));
         flow.animation->initializePositions(_deferred_buffer->depthTexture(), min, max);

         activeDevice().setShader(_flow_init_param_shader);
         activeDevice().bindSampler(_flow_init_param_depth, 0);
         activeDevice().setParameter(_flow_init_param_inv_proj, inv_proj);
         // the original set this vec2 with a 3-component Vector (GL_INVALID_OPERATION), so it
         // always stayed (0, 0) - keep that, it selects the flow field offsets.
         activeDevice().setParameter(_flow_init_param_center, Vector2(0.0f, 0.0f));
         flow.animation->initializeParams(_deferred_buffer->depthTexture(), min, max);

         activeDevice().setShader(0);
      }

      activeDevice().setShader(_flow_update_pos_shader);
      activeDevice().bindSampler(_flow_depth_texture, 0);
      activeDevice().bindSampler(_flow_vertex_pos_texture, 1);
      activeDevice().bindSampler(_flow_vertex_param_texture, 2);
      activeDevice().bindSampler(_flow_field_texture, 3);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _deferred_buffer->depthTexture());

      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, _flow_field_texture_id);

      activeDevice().setParameter(_flow_center, flow.animation->getCenter());
      activeDevice().setParameter(_flow_field_scale, flow.animation->getScale());
      activeDevice().setParameter(_flow_time_delta, _delta);
      activeDevice().setParameter(_flow_src_rect, Vector4(min.x, min.y, max.x, max.y));
      activeDevice().setParameter(_flow_inv_proj, inv_proj);
      activeDevice().setParameter(_flow_stop, flow.animation->stopped() ? 1.0f : 0.0f);

      flow.animation->updatePositions(_delta);

      activeDevice().setShader(_flow_update_col_shader);
      activeDevice().bindSampler(_flow_update_col_color_map, 0);
      activeDevice().bindSampler(_flow_update_col_position_map, 1);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _deferred_buffer->texture());

      activeDevice().setParameter(_flow_update_col_proj, proj);
      activeDevice().setParameter(_flow_update_col_stop, flow.animation->stopped() ? 1.0f : 0.0f);

      flow.animation->updateColors();

      activeDevice().setShader(0);

      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
   }

   glActiveTexture(GL_TEXTURE0);

   FrameBuffer::pop();

   // particle positions are already baked into world space - identity world transform, matching
   // PlayerDeathEffect::render()'s own convention for the same situation.
   activeDevice().setShader(_points_shader);
   activeDevice().setParameter(_points_projection, proj);
   activeDevice().push(Matrix());

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_FALSE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _particle_texture_id);
   activeDevice().bindSampler(_points_texture, 0);

   // the original always rendered into a screen-sized FrameBuffer; without one, scale by the
   // screen width so particle size keeps its 1920 px reference.
   float size_factor = static_cast<float>(activeDevice().getWidth()) / 1920.0f;
   const auto fb = FrameBuffer::Instance();
   if (fb)
   {
      size_factor = fb->get().getSizeFactor(1920.0f);
   }

   for (const auto& flow : _flow_animations)
   {
      activeDevice().setParameter(_points_size, flow.animation->getPointSize() * size_factor);
      flow.animation->draw();
   }

   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);

   activeDevice().pop();
   activeDevice().setShader(0);
}
