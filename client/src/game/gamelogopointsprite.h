#pragma once

// Draws a screen-facing quad batch (not real GL point sprites, despite the class name).

#include <cstdint>
#include <vector>
#include "math/vector.h"
#include "render/texture.h"

class GameLogoPointSprite
{
public:
   //! constructor
   GameLogoPointSprite();

   //! initialize texture
   static void initialize();

   //! setter for point sprite vectors
   static void setPointSprites(std::vector<Vector> v, std::vector<float> glow_values);

   //! draw the vectors in one go
   static void draw();

protected:
   //! pointsprite texture
   static Texture sTexture;

   //! shader + uniform locations
   static uint32_t sShader;
   static int sTextureParam;

   //! dynamic quad-batch vertex buffer (position + texcoord per vertex, 6 verts/sprite -
   //! rebuilt every draw() call since positions/glow values change every frame)
   static uint32_t sVertexBuffer;

   //! vector singleton
   static std::vector<Vector> _positions;

   //! glow values
   static std::vector<float> _glow_values;
};
