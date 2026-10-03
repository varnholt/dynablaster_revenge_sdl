#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

// offscreen render target (color texture plus optional depth renderbuffer or sampleable depth texture),
// optionally multisampled: drawn into renderbuffers then, resolved into the texture by unbind()
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

   // remembers the bound frame buffer for pop(), binds "frame_buffer" if given
   static void push();
   static void push(FrameBuffer& frame_buffer);
   static void pop();
   // the bound frame buffer, empty while drawing to the screen
   static std::optional<std::reference_wrapper<FrameBuffer>> Instance();

   // the frame everything is drawn into before it's presented, unbind() and pop() return to it
   static void setScreen(FrameBuffer& frame_buffer);
   static void clearScreen();
   static uint32_t screenTarget();

   // glCopyTexImage2D() of the bound framebuffer into the bound texture, resolving a multisampled one first
   static void copyTexImage(int32_t x, int32_t y, int32_t width, int32_t height);

   int32_t width() const;
   int32_t height() const;
   // width * sqrt(samples) / reference_width, point sizes grow with the samples like the original
   float getSizeFactor(float reference_width) const;
   // multi_sample -1 keeps the current sample count
   bool setResolution(int32_t width, int32_t height, int32_t multi_sample = -1);
   bool resolutionChanged(int32_t width, int32_t height, int32_t multi_sample = -1) const;
   // the samples the driver actually gave, 1 without multisampling
   int32_t samples() const;
   uint32_t texture() const;
   uint32_t target() const;
   uint32_t depthTexture() const;
   void bind(int32_t width = 0, int32_t height = 0);
   void unbind();

   void draw(float alpha);

private:
   void discard();
   void discardMultisampleTarget();
   void resolve();
   void createMultisampleTarget(int32_t width, int32_t height);

   uint32_t _target = 0;
   uint32_t _texture = 0;
   uint32_t _depth_buffer = 0;
   uint32_t _depth_texture = 0;
   int32_t _width = 0;
   int32_t _height = 0;
   int32_t _format_flags = 0;

   uint32_t _multisample_target = 0;
   uint32_t _multisample_color = 0;
   uint32_t _multisample_depth = 0;
   int32_t _requested_samples = 0;
   int32_t _samples = 1;

   static inline std::optional<std::reference_wrapper<FrameBuffer>> _instance;
   static inline std::optional<std::reference_wrapper<FrameBuffer>> _screen;
   static inline std::vector<std::optional<std::reference_wrapper<FrameBuffer>>> _stack;

   // full-screen quad used by draw(), lazily created, kept for the process lifetime
   static inline uint32_t _quad_vertex_buffer = 0;
};
