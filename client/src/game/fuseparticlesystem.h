#pragma once

/// \brief GLES3-native rebuild of client/src/game/fuseparticle.{h,cpp} + fuseparticleemitter.{h,cpp}
/// (bomb fuse sparks, deferred during the Qt->SDL port - see project memory). The original modeled
/// each spark as its own heap-allocated FuseParticle drawn via glBegin(GL_QUADS); GLES3 has no
/// immediate mode, so this is one manager owning a single dynamic point-sprite VBO shared by every
/// active bomb's sparks, drawn in one glDrawArrays(GL_POINTS, ...) call per frame. Per-particle
/// physics (direction/scalar/pointSize decay, respawn-on-burnout, fast burn-down when a bomb is
/// kicked far from its expected position) is unchanged from the original.

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
