#pragma once

// materials call raw gl* functions directly, so the GLES3 declarations are exposed here too
#include "../gles3.h"
#include "renderdevice.h"

#include <cstdint>
#include <map>

// GLES3 render device. The matrices are computed on the CPU and uploaded as the uniforms
// "u_modelView", "u_projection", "u_normalMatrix" and "u_modelViewProjection" to whichever of those
// a shader declares. The camera matrix is folded into the projection slot (see setCamera()).
class GLDevice : public RenderDevice
{
public:
   bool init() override;

   void resize(int32_t x, int32_t y) override;
   void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) override;
   void getViewPort(int32_t* x, int32_t* y, int32_t* width, int32_t* height) override;
   void convertFromViewPort(int32_t* x, int32_t* y, int32_t target_width, int32_t target_height) override;
   void clear() override;

   // clears with an explicit color (e.g. alpha=0 for offscreen buffers blended later),
   // then restores the default clear color
   void clear(float r, float g, float b, float a);

   void setPerspective(float fov, float aspect, float z_near = 1.0f, float z_far = 5000.0f) override;
   void setCamera(const Matrix& matrix, float fov, float z_near = 1.0f, float z_far = 1000.0f, bool perspective = true) override;
   void push(const Matrix& matrix) override;
   void pop() override;

   // combined projection*view matrix computed by the last setCamera()
   Matrix getProjectionMatrix() const
   {
      return _projection_matrix;
   }

   // single-slot save/restore of the projection matrix around a temporary setCamera() (not nestable)
   void pushProjection()
   {
      _saved_projection_matrix = _projection_matrix;
   }

   void popProjection()
   {
      _projection_matrix = _saved_projection_matrix;
   }

   // assigns the projection matrix directly, used by 2D screen-space rendering
   void setProjectionMatrix(const Matrix& matrix)
   {
      _projection_matrix = matrix;
   }

   uint32_t createVertexBuffer(int32_t size, bool dynamic = false) override;
   void allocateVertexBuffer(uint32_t buffer, int32_t size, bool dynamic = false) override;
   void* lockVertexBuffer(uint32_t handle, int32_t size = 0) override;
   void unlockVertexBuffer(uint32_t buffer) override;

   uint32_t createIndexBuffer(int32_t size, bool dynamic = false) override;
   void allocateIndexBuffer(uint32_t buffer, int32_t size, bool dynamic = false) override;
   void* lockIndexBuffer(uint32_t handle, int32_t size = 0) override;
   void unlockIndexBuffer(uint32_t buffer) override;
   void setCulling(bool state) override;
   void setMaterial(const Vector& ambient, const Vector& diffuse, const Vector& specular, float shine) override;

   void drawLine(Vector* vertices) override;

   uint32_t createTexture(void* data, int32_t x, int32_t y, int32_t flags = 3) override;
   void deleteTexture(uint32_t texture_id) override;
   void updateTexture(void* data, int32_t x, int32_t y, int32_t flags) override;

   uint32_t uploadTexture1D(void* data, int32_t x, int32_t flags = 0) override;
   uint32_t loadShader(const char* vertex_name, const char* fragment_name) override;
   void setShader(uint32_t shader) override;
   int32_t getParameterIndex(const char* name) override;
   void bindSampler(int32_t position, int32_t unit) override;
   void setParameter(int32_t position, float* data, int32_t size) override;
   void setParameter(int32_t position, const Vector& vector) override;
   void setParameter(int32_t position, const Vector2& vector) override;
   void setParameter(int32_t position, const Vector4& vector) override;
   void setParameter(int32_t position, const Matrix& matrix) override;
   void setParameter(int32_t position, const Matrix* matrix, int32_t count) override;
   void setParameter(int32_t position, float value) override;

   uint32_t createBuffer() override;
   void deleteBuffer(uint32_t buffer) override;

   void setSwapInterval(int32_t interval) override;

private:
   // a linked program plus the reserved-name uniform locations push()/setCamera() feed
   struct ShaderInfo
   {
      uint32_t program = 0;
      int32_t model_view_location = -1;
      int32_t projection_location = -1;
      int32_t normal_matrix_location = -1;
      int32_t model_view_projection_location = -1;
   };

   void uploadTransformUniforms();

   std::map<uint32_t, ShaderInfo> _shader_table;
   uint32_t _shader_alloc_index = 0;

   int32_t _viewport_x = 0;
   int32_t _viewport_y = 0;
   int32_t _viewport_width = 0;
   int32_t _viewport_height = 0;

   Matrix _projection_matrix;
   Matrix _saved_projection_matrix;
   Matrix _world_transform;

   // glMapBufferRange() needs an explicit length; lock*Buffer() without a size maps the last allocated size
   int32_t _last_vertex_buffer_size = 0;
   int32_t _last_index_buffer_size = 0;
};
