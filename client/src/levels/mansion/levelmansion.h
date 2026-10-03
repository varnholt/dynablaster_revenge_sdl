#pragma once

#include "../level.h"
#include "materials/materialfactory.h"

class Camera;

class LevelMansion : public Level, public MaterialFactory
{
public:
   LevelMansion();

   void loadData() override;
   std::string getLensFlareKey() const override;

   std::unique_ptr<Material> createMaterial(int32_t id) const override;
};
