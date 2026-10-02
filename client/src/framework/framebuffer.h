#pragma once

#include <cstdint>
#include <vector>

// offscreen render target (color texture plus optional depth renderbuffer or sampleable depth texture)
class FrameBuffer
{
public:
   enum FormatFlags
   {
      NoDepthBuffer = 1,
      DepthTexture = 2
   };

   FrameBuffer(int32_t width, int32_t height, int32_t multi_sample = 0, int32_t format_flags = 0);
   ~FrameBuffer();

   FrameBuffer(const FrameBuffer&) = delete;
   FrameBuffer& operator=(const FrameBuffer&) = delete;

   static void push(FrameBuffer* frame_buffer = nullptr);
   static void pop();
   static FrameBuffer* Instance();

   // the frame everything is drawn into before it's presented, unbind() and pop() return to it
   static void setScreen(FrameBuffer* frame_buffer);
   static uint32_t screenTarget();

   int32_t width() const;
   int32_t height() const;
   // width / refWidth (no multisampling, so no sqrt(samples) factor)
   float getSizeFactor(float reference_width) const;
   bool setResolution(int32_t width, int32_t height);
   bool resolutionChanged(int32_t width, int32_t height) const;
   uint32_t texture() const;
   uint32_t target() const;
   uint32_t depthTexture() const;
   void bind(int32_t width = 0, int32_t height = 0);
   void unbind();

   void draw(float alpha);

private:
   void discard();

   uint32_t _target = 0;
   uint32_t _texture = 0;
   uint32_t _depth_buffer = 0;
   uint32_t _depth_texture = 0;
   int32_t _width = 0;
   int32_t _height = 0;
   int32_t _format_flags = 0;

   static inline FrameBuffer* _instance = nullptr;
   static inline FrameBuffer* _screen = nullptr;
   static inline std::vector<FrameBuffer*> _stack;

   // full-screen quad used by draw(), lazily created, kept for the process lifetime
   static inline uint32_t _quad_vertex_buffer = 0;
};
