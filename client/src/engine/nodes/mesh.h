#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "../render/geometry.h"
#include "node.h"
#include "tools/list.h"

class Stream;
class MotionMixer;

class Mesh : public Node
{
public:
   Mesh(Node* parent = nullptr);
   Mesh(const Mesh& mesh, Node* parent = nullptr);

   void copy(const Mesh& mesh);

   void load(Stream* stream) override;
   void write(Stream* stream) override;

   int32_t getPartCount() const;
   void add(Geometry* geometry);
   Geometry* getPart(int32_t index) const;

   void setAnimationFrame(float frame);
   float getAnimationFrame() const;
   Node* getSkeleton() const;
   void setSkeleton(Node* node);

   MotionMixer* getMotionMixer() const;
   void setMotionMixer(MotionMixer* mixer);

   void transform(float frame) override;

   uint32_t getRenderFlags() const;
   void setRenderFlags(uint32_t flags);
   float getRenderParameter(int32_t index) const;
   void setRenderParameter(int32_t index, float param);

   void createBoxMapping(bool unwrap, const Vector& min, const Vector& max, const Matrix& gizmo = Matrix());

protected:
   // geometries are not owned: materials and vertex buffer pools keep raw pointers to them
   std::vector<Geometry*> _geometry;
   Node* _skeleton = nullptr;
   MotionMixer* _motion_mixer = nullptr;  // not owned
   float _animation_frame = 0.0f;
   uint32_t _render_flags = 0;
   std::array<float, 4> _render_parameter{};
};
