#pragma once

/// \brief Bomb fuse sparks: one manager owns a single point-sprite VBO shared by every active bomb.

#include <unordered_map>
#include <vector>

#include "math/vector.h"

class MapItem;

class FuseParticleSystem
{
public:
   FuseParticleSystem();
   ~FuseParticleSystem();

   //! start emitting sparks for a newly placed bomb
   void addEmitter(MapItem* item, const Vector& origin);

   //! update a bomb's current position (sparks catch up gradually; a large jump - a kicked bomb -
   //! burns the current sparks down quickly instead of dragging them across the map)
   void setEmitterPosition(MapItem* item, const Vector& origin);

   //! bomb detonated/removed - existing sparks finish their current arc instead of respawning
   void removeEmitter(MapItem* item);

   void animate(float dt);
   void render();

   //! offset from a bomb's mesh origin to where its fuse actually sits
   static const Vector& getBombOffset();

private:
   struct Particle
   {
      Vector origin;
      Vector direction;
      Vector position;
      float pointSize;
      float scalar;
      float elapsed;
      float randomStartTime;
      bool started;
   };

   struct Emitter
   {
      Vector origin;
      Vector nextOrigin;
      std::vector<Particle> particles;
      bool removing;
   };

   void resetParticle(Particle& particle, const Vector& origin);

   std::unordered_map<MapItem*, Emitter> mEmitters;

   std::vector<float> mUploadBuffer;
   unsigned int mVertexBuffer;
   unsigned int mParticleTextureId;
   unsigned int mShader;
   int mTexture;
   int mPointSize;
   int mProjection;
};
