#pragma once

#include <cstdint>
#include <memory>
#include "material.h"

class MaterialFactory
{
public:
   virtual ~MaterialFactory() = default;

   // nullptr for material ids the factory does not handle
   virtual std::unique_ptr<Material> createMaterial(int32_t material_id) const = 0;
};
