#pragma once

#include "engine/materials/materialfactory.h"

/// \brief MaterialFactory for the harness demo scene, same material-ID dispatch as
/// LevelCastle::createMaterial.
class DemoMaterialFactory : public MaterialFactory
{
public:
   std::unique_ptr<Material> createMaterial(int32_t material_id) const override;
};
