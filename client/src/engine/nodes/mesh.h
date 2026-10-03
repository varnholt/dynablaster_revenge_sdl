#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include "../render/geometry.h"
#include "animation/motionmixer.h"
#include "node.h"

class Stream;

class Mesh : public Node
{
public:
   Mesh();
   Mesh(const Mesh& mesh);  // the parts share the vertex data of "mesh"
   ~Mesh() override;

   void copy(const Mesh& mesh);  // deep copy of the parts of "mesh"

   void load(Stream& stream) override;
   void write(Stream& stream) override;

   int32_t getPartCount() const;
   Geometry& addPart();
   Geometry& getPart(int32_t index) const;

   void setAnimationFrame(float frame);
   float getAnimationFrame() const;
   std::optional<std::reference_wrapper<Node>> getSkeleton() const;
   void setSkeleton(Node& node);

   std::optional<std::reference_wrapper<MotionMixer>> getMotionMixer() const;
   void setMotionMixer(std::unique_ptr<MotionMixer> mixer);

   // adds the scene's start indices to the loaded material and bone ids
   void offsetIds(int32_t material_offset, int32_t node_offset);

   void transform(float frame) override;

   uint32_t getRenderFlags() const;
   void setRenderFlags(uint32_t flags);
   float getRenderParameter(int32_t index) const;
   void setRenderParameter(int32_t index, float param);

   void createBoxMapping(bool unwrap, const Vector& min, const Vector& max, const Matrix& gizmo = Matrix());

protected:
   std::vector<std::unique_ptr<Geometry>> _geometry;
   std::optional<std::reference_wrapper<Node>> _skeleton;
   std::unique_ptr<MotionMixer> _motion_mixer;
   float _animation_frame = 0.0f;
   uint32_t _render_flags = 0;
   std::array<float, 4> _render_parameter{};
};
