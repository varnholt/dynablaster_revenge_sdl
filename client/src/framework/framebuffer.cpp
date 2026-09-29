#include "framebuffer.h"
#include "gldevice.h"

#include <array>

FrameBuffer::FrameBuffer(int32_t width, int32_t height, int32_t /*multi_sample*/, int32_t format_flags) : _format_flags(format_flags)
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

FrameBuffer* FrameBuffer::Instance()
{
   return _instance;
}

void FrameBuffer::discard()
{
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

bool FrameBuffer::setResolution(int32_t width, int32_t height)
{
   if (_width == width && _height == height)
   {
      return true;
   }

   discard();

   glGenTextures(1, &_texture);
   glBindTexture(GL_TEXTURE_2D, _texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &_target);
   glBindFramebuffer(GL_FRAMEBUFFER, _target);

   if ((_format_flags & NoDepthBuffer) == 0)
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

   if ((_format_flags & NoDepthBuffer) == 0)
   {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }
   else
   {
      glClear(GL_COLOR_BUFFER_BIT);
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return ok;
}

void FrameBuffer::bind(int32_t width, int32_t height)
{
   _instance = this;
   glBindFramebuffer(GL_FRAMEBUFFER, _target);
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
   _instance = nullptr;
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::push(FrameBuffer* frame_buffer)
{
   _stack.push_back(_instance);
   if (frame_buffer)
   {
      frame_buffer->bind();
   }
}

void FrameBuffer::pop()
{
   if (!_stack.empty())
   {
      FrameBuffer* previous = _stack.back();
      _stack.pop_back();
      if (previous)
      {
         previous->bind();
      }
      else
      {
         _instance = nullptr;
         glBindFramebuffer(GL_FRAMEBUFFER, 0);
         glViewport(activeDevice->getBorderLeft(), activeDevice->getBorderBottom(), activeDevice->getWidth(), activeDevice->getHeight());
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
   return static_cast<float>(_width) / reference_width;
}

bool FrameBuffer::resolutionChanged(int32_t width, int32_t height) const
{
   return (_width != width || _height != height);
}

uint32_t FrameBuffer::texture() const
{
   return _texture;
}

uint32_t FrameBuffer::target() const
{
   return _target;
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

   activeDevice->setParameter(activeDevice->getParameterIndex("alpha"), alpha);

   glBindBuffer(GL_ARRAY_BUFFER, _quad_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<const GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}
