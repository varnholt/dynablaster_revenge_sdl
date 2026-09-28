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

   virtual void initialize();
   virtual void draw();
   virtual void animate(float dt);
   virtual void reset();
   virtual void loadData();
   std::string getLensFlareKey() const override;

   Material* createMaterial(SceneGraph* scene, int id) const;

private:
   std::unique_ptr<SnowAnimation> _snow_animation;
};
