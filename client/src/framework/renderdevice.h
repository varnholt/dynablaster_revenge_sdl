#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "math/matrix.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/vector4.h"

class Stream;

class RenderDevice
{
public:
   struct ViewPort
   {
      int32_t x = 0;
      int32_t y = 0;
      int32_t width = 0;
      int32_t height = 0;
   };

   // the most recently constructed device becomes the active one
   RenderDevice();
   virtual ~RenderDevice();

   RenderDevice(const RenderDevice&) = delete;
   RenderDevice& operator=(const RenderDevice&) = delete;

   void setKey(int32_t num, int32_t state);
   int32_t getKey(int32_t num);

   virtual bool abort();
   virtual void setAbort();
   virtual bool active();
   virtual void setActive(bool state);
   virtual Matrix getCameraMatrix() const;

   virtual bool init() = 0;
   virtual void clear() = 0;
   virtual void resize(int32_t x, int32_t y) = 0;
   virtual void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
   virtual ViewPort getViewPort() const = 0;
   virtual void convertFromViewPort(int32_t& x, int32_t& y, int32_t target_width, int32_t target_height) = 0;

   virtual void setPerspective(float fov, float aspect, float z_near, float z_far) = 0;
   virtual void setCamera(const Matrix& matrix, float fov, float z_near, float z_far, bool perspective = false) = 0;
   virtual void push(const Matrix& matrix) = 0;
   virtual void pop() = 0;

   virtual uint32_t createVertexBuffer(int32_t size, bool dynamic = false) = 0;
   virtual void allocateVertexBuffer(uint32_t buffer, int32_t size, bool dynamic = false) = 0;
   virtual std::span<std::byte> mapVertexBuffer(uint32_t handle, int32_t size = 0) = 0;
   virtual void unlockVertexBuffer(uint32_t buffer) = 0;

   virtual uint32_t createIndexBuffer(int32_t size, bool dynamic = false) = 0;
   virtual void allocateIndexBuffer(uint32_t buffer, int32_t size, bool dynamic = false) = 0;
   virtual std::span<std::byte> mapIndexBuffer(uint32_t handle, int32_t size = 0) = 0;
   virtual void unlockIndexBuffer(uint32_t buffer) = 0;

   // maps the buffer as an array of T; without a size the last allocated buffer size is mapped
   template <class T>
   std::span<T> lockVertexBuffer(uint32_t handle, int32_t size = 0)
   {
      return asSpan<T>(mapVertexBuffer(handle, size));
   }

   template <class T>
   std::span<T> lockIndexBuffer(uint32_t handle, int32_t size = 0)
   {
      return asSpan<T>(mapIndexBuffer(handle, size));
   }

   virtual void setCulling(bool state) = 0;
   virtual void setMaterial(const Vector& ambient, const Vector& diffuse, const Vector& specular, float shine) = 0;

   virtual uint32_t createTexture(std::span<const uint32_t> data, int32_t x, int32_t y, int32_t flags = 3) = 0;
   virtual void deleteTexture(uint32_t texture_id) = 0;
   virtual void updateTexture(std::span<const uint32_t> data, int32_t x, int32_t y, int32_t flags) = 0;
   virtual uint32_t loadShader(const std::string& vertex_name, const std::string& fragment_name) = 0;
   virtual void setShader(uint32_t shader) = 0;
   virtual int32_t getParameterIndex(const std::string& name) = 0;
   virtual void bindSampler(int32_t position, int32_t unit) = 0;
   virtual void setParameter(int32_t position, std::span<const float> data) = 0;
   virtual void setParameter(int32_t position, const Vector& value) = 0;
   virtual void setParameter(int32_t position, const Vector2& value) = 0;
   virtual void setParameter(int32_t position, const Vector4& value) = 0;
   virtual void setParameter(int32_t position, const Matrix& value) = 0;
   virtual void setParameter(int32_t position, std::span<const Matrix> values) = 0;
   virtual void setParameter(int32_t position, float value) = 0;

   virtual uint32_t createBuffer() = 0;
   virtual void deleteBuffer(uint32_t buffer) = 0;

   virtual void setSwapInterval(int32_t interval) = 0;

   virtual float getWidth() const;
   virtual float getHeight() const;

   void setBorder(int32_t left, int32_t top, int32_t right, int32_t bottom);
   int32_t getBorderLeft() const;
   int32_t getBorderBottom() const;

protected:
   int32_t _width = 0;
   int32_t _height = 0;
   float _aspect = 16.0f / 9.0f;
   int32_t _active = 0;
   int32_t _abort = 0;
   int32_t _time = 0;
   Matrix _camera;
   uint32_t _current_shader = 0;
   std::array<int32_t, 256> _keys{};  // keyboard state
   int32_t _border_left = 0;
   int32_t _border_top = 0;
   int32_t _border_right = 0;
   int32_t _border_bottom = 0;

private:
   template <class T>
   static std::span<T> asSpan(std::span<std::byte> bytes)
   {
      return {reinterpret_cast<T*>(bytes.data()), bytes.size() / sizeof(T)};
   }
};

// the active render device, only valid while one exists
RenderDevice& activeDevice();
