#pragma once

// base
#include "../level.h"
#include "materials/materialfactory.h"

#include <memory>

class SnowAnimation;

class LevelCastle : public Level, public MaterialFactory
{
public:
   LevelCastle();
   ~LevelCastle() override;

   void initialize() override;
   void draw() override;
   void animate(float dt) override;
   void reset() override;
   void loadData() override;
   std::string getLensFlareKey() const override;

   Material* createMaterial(SceneGraph* scene, int id) const override;

private:
   std::unique_ptr<SnowAnimation> _snow_animation;
};
