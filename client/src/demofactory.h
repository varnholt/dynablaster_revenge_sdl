#pragma once

#include "engine/materials/materialfactory.h"

/// \brief MaterialFactory for the harness demo scene, same material-ID dispatch as
/// LevelCastle::createMaterial.
class DemoMaterialFactory : public MaterialFactory
{
public:
   Material* createMaterial(SceneGraph* scene, int materialId) const override;
};
