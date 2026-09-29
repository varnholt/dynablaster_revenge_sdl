#pragma once

#include <cstdint>
#include "material.h"

class SceneGraph;

class MaterialFactory
{
public:
   virtual ~MaterialFactory() = default;

   // the created material registers itself with (and is owned by) the given scene
   virtual Material* createMaterial(SceneGraph* scene, int32_t material_id) const = 0;
};
