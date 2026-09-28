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

   Material* createMaterial(SceneGraph* scene, int id) const override;

private:
   void loadDestructions(Camera* shadow_camera);
};
