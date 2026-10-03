#include "demofactory.h"

#include "engine/materials/displacementmaterial.h"
#include "engine/materials/environmentambientdiffusematerial.h"
#include "engine/materials/environmentmaterial.h"
#include "engine/materials/environmenttexturematerial.h"
#include "engine/materials/material.h"
#include "engine/materials/texturematerial.h"

std::unique_ptr<Material> DemoMaterialFactory::createMaterial(int32_t material_id) const
{
   switch (material_id)
   {
      case MAP_DIFFUSE:
         return std::make_unique<TextureMaterial>();
      case MAP_REFLECT:
         return std::make_unique<EnvironmentMaterial>();
      case (MAP_DIFFUSE | MAP_REFLECT):
         return std::make_unique<EnvironmentTextureMaterial>();
      case (MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT):
         return std::make_unique<EnvironmentAmbientDiffuseMaterial>();
      case (MAP_DIFFUSE | MAP_DISPLACE):
         return std::make_unique<DisplacementMaterial>();
      default:
         return nullptr;
   }
}
