#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include "vertexbuffer.h"

class Geometry;

// one vertex buffer per geometry id, shared by all geometry copies with that id
class VertexBufferPool
{
public:
   bool contains(const Geometry& geometry) const;
   VertexBuffer& add(const Geometry& geometry);
   std::optional<std::reference_wrapper<VertexBuffer>> get(const Geometry& geometry);

private:
   std::unordered_map<int32_t, std::unique_ptr<VertexBuffer>> _pool;
};
