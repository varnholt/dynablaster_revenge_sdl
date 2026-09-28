#pragma once

#include "../level.h"
#include "materials/materialfactory.h"

#include <memory>

class Camera;
class SpaceBackground;

class LevelSpace : public Level, public MaterialFactory
{
public:
   LevelSpace();
   ~LevelSpace() override;

   void loadData() override;
   void drawBackground() override;
   void animate(float dt) override;
   std::string getLensFlareKey() const override;

   Material* createMaterial(SceneGraph* scene, int id) const override;

   std::unique_ptr<SpaceBackground> _background;
};
