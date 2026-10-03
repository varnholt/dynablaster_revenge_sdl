#include "framebuffer.h"
#include "gldevice.h"

#include <algorithm>
#include <array>
#include <cmath>

FrameBuffer::FrameBuffer(int32_t width, int32_t height, int32_t multi_sample, int32_t format_flags)
    : _format_flags(format_flags), _requested_samples(multi_sample)
{
   if (setResolution(width, height))
   {
      _width = width;
      _height = height;
   }
}

FrameBuffer::~FrameBuffer()
{
   discard();
}

std::optional<std::reference_wrapper<FrameBuffer>> FrameBuffer::Instance()
{
   return _instance;
}

void FrameBuffer::setScreen(FrameBuffer& frame_buffer)
{
   _screen = frame_buffer;
}

void FrameBuffer::clearScreen()
{
   _screen.reset();
}

uint32_t FrameBuffer::screenTarget()
{
   return _screen ? _screen->get().target() : 0;
}

void FrameBuffer::copyTexImage(int32_t x, int32_t y, int32_t width, int32_t height)
{
   GLint bound = 0;
   glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);

   // a multisampled framebuffer can't be read, its resolved texture can
   std::optional<std::reference_wrapper<FrameBuffer>> source;
   for (const auto& frame_buffer : {_instance, _screen})
   {
      if (frame_buffer && frame_buffer->get()._multisample_target != 0 &&
          static_cast<GLint>(frame_buffer->get()._multisample_target) == bound)
      {
         source = frame_buffer;
         break;
      }
   }

   if (source)
   {
      source->get().resolve();
      glBindFramebuffer(GL_READ_FRAMEBUFFER, source->get()._target);
   }

   glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, width, height, 0);

   if (source)
   {
      glBindFramebuffer(GL_FRAMEBUFFER, source->get()._multisample_target);
   }
}

void FrameBuffer::discardMultisampleTarget()
{
   if (_multisample_target)
   {
      glDeleteFramebuffers(1, &_multisample_target);
   }

   if (_multisample_color)
   {
      glDeleteRenderbuffers(1, &_multisample_color);
   }

   if (_multisample_depth)
   {
      glDeleteRenderbuffers(1, &_multisample_depth);
   }

   _multisample_target = 0;
   _multisample_color = 0;
   _multisample_depth = 0;
   _samples = 1;
}

void FrameBuffer::discard()
{
   discardMultisampleTarget();

   if (_target)
   {
      glDeleteFramebuffers(1, &_target);
   }

   if (_depth_buffer)
   {
      glDeleteRenderbuffers(1, &_depth_buffer);
   }

   if (_depth_texture)
   {
      glDeleteTextures(1, &_depth_texture);
   }

   if (_texture)
   {
      glDeleteTextures(1, &_texture);
   }

   _target = 0;
   _depth_buffer = 0;
   _depth_texture = 0;
   _texture = 0;
}

bool FrameBuffer::setResolution(int32_t width, int32_t height, int32_t multi_sample)
{
   if (!resolutionChanged(width, height, multi_sample))
   {
      return true;
   }

   if (multi_sample >= 0)
   {
      _requested_samples = multi_sample;
   }

   discard();

   // the multisampled renderbuffers carry the depth, this target only receives the resolve
   const bool multisampled = _requested_samples > 1;

   glGenTextures(1, &_texture);
   glBindTexture(GL_TEXTURE_2D, _texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   // a resolve needs the exact format of the multisampled color buffer
   glTexImage2D(GL_TEXTURE_2D, 0, multisampled ? GL_RGBA8 : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &_target);
   glBindFramebuffer(GL_FRAMEBUFFER, _target);

   if ((_format_flags & NoDepthBuffer) == 0 && !multisampled)
   {
      if (_format_flags & DepthTexture)
      {
         // sampleable depth buffer (PlayerDeathEffect reads it back to seed particle positions)
         glGenTextures(1, &_depth_texture);
         glBindTexture(GL_TEXTURE_2D, _depth_texture);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
         glBindTexture(GL_TEXTURE_2D, 0);
         glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth_texture, 0);
      }
      else
      {
         glGenRenderbuffers(1, &_depth_buffer);
         glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
         glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
         glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buffer);
      }
   }

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);

   const bool ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   if (ok)
   {
      _width = width;
      _height = height;
   }
   else
   {
      _width = 0;
      _height = 0;
   }

   if (ok && multisampled)
   {
      createMultisampleTarget(width, height);
   }

   if ((_format_flags & NoDepthBuffer) == 0)
   {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }
   else
   {
      glClear(GL_COLOR_BUFFER_BIT);
   }

   // a buffer created mid-frame mustn't leave the frame unbound
   glBindFramebuffer(GL_FRAMEBUFFER, _instance ? _instance->get().target() : screenTarget());

   return ok;
}

void FrameBuffer::createMultisampleTarget(int32_t width, int32_t height)
{
   glGenFramebuffers(1, &_multisample_target);
   glBindFramebuffer(GL_FRAMEBUFFER, _multisample_target);

   GLint max_samples = 0;
   glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

   glGenRenderbuffers(1, &_multisample_color);
   glBindRenderbuffer(GL_RENDERBUFFER, _multisample_color);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, std::min(_requested_samples, max_samples), GL_RGBA8, width, height);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _multisample_color);

   GLint samples = 0;
   glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);

   // no sampleable depth here, GLES 3.0 has no multisampled textures
   if ((_format_flags & NoDepthBuffer) == 0)
   {
      glGenRenderbuffers(1, &_multisample_depth);
      glBindRenderbuffer(GL_RENDERBUFFER, _multisample_depth);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _multisample_depth);
   }

   if (samples > 1 && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
   {
      _samples = samples;
      return;
   }

   // the driver refused: draw without multisampling, into the resolve target plus a depth buffer
   discardMultisampleTarget();

   glBindFramebuffer(GL_FRAMEBUFFER, _target);
   if ((_format_flags & NoDepthBuffer) == 0)
   {
      glGenRenderbuffers(1, &_depth_buffer);
      glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buffer);
   }
}

void FrameBuffer::resolve()
{
   if (_multisample_target == 0)
   {
      return;
   }

   glBindFramebuffer(GL_READ_FRAMEBUFFER, _multisample_target);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _target);
   glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void FrameBuffer::bind(int32_t width, int32_t height)
{
   _instance = *this;
   glBindFramebuffer(GL_FRAMEBUFFER, target());
   if (width && height)
   {
      glViewport(0, 0, width, height);
   }
   else
   {
      glViewport(0, 0, _width, _height);
   }
}

void FrameBuffer::unbind()
{
   resolve();

   if (_screen && &_screen->get() != this)
   {
      _screen->get().bind();
      return;
   }

   _instance.reset();
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::push()
{
   _stack.push_back(_instance);
}

void FrameBuffer::push(FrameBuffer& frame_buffer)
{
   push();
   frame_buffer.bind();
}

void FrameBuffer::pop()
{
   if (!_stack.empty())
   {
      const auto previous = _stack.back();
      _stack.pop_back();
      if (previous)
      {
         previous->get().bind();
      }
      else if (_screen)
      {
         _screen->get().bind();
      }
      else
      {
         _instance.reset();
         glBindFramebuffer(GL_FRAMEBUFFER, 0);
         glViewport(
            activeDevice().getBorderLeft(), activeDevice().getBorderBottom(), activeDevice().getWidth(), activeDevice().getHeight()
         );
      }
   }
}

int32_t FrameBuffer::width() const
{
   return _width;
}

int32_t FrameBuffer::height() const
{
   return _height;
}

float FrameBuffer::getSizeFactor(float reference_width) const
{
   return static_cast<float>(_width) * std::sqrt(static_cast<float>(_samples)) / reference_width;
}

bool FrameBuffer::resolutionChanged(int32_t width, int32_t height, int32_t multi_sample) const
{
   return _width != width || _height != height || (multi_sample >= 0 && multi_sample != _requested_samples);
}

int32_t FrameBuffer::samples() const
{
   return _samples;
}

uint32_t FrameBuffer::texture() const
{
   return _texture;
}

uint32_t FrameBuffer::target() const
{
   return _multisample_target != 0 ? _multisample_target : _target;
}

uint32_t FrameBuffer::depthTexture() const
{
   return _depth_texture;
}

void FrameBuffer::draw(float alpha)
{
   // positions are in clip space: the caller binds the shader and sets an identity world transform
   // and projection (GLDevice::setProjectionMatrix()) first
   if (_quad_vertex_buffer == 0)
   {
      static constexpr std::array<float, 30> quad = {
         -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, -1.0f, 1.0f, 1.0f,
         -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
      };

      glGenBuffers(1, &_quad_vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, _quad_vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad.data(), GL_STATIC_DRAW);
   }

   glBindTexture(GL_TEXTURE_2D, _texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   activeDevice().setParameter(activeDevice().getParameterIndex("alpha"), alpha);

   glBindBuffer(GL_ARRAY_BUFFER, _quad_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<const GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}
