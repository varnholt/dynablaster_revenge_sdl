#pragma once

// base
#include "../level.h"
#include "materials/materialfactory.h"

// Not ported yet: SnowAnimation (immediate-mode GL).
class LevelCastle : public Level, public MaterialFactory
{

public:

   LevelCastle();

   virtual void initialize();
   virtual void draw();
   virtual void animate(float dt);
   virtual void reset();
   virtual void loadData();
   std::string getLensFlareKey() const override;

   Material* createMaterial(SceneGraph* scene, int id) const;
};
