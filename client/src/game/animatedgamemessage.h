#pragma once

// GLES3 port of client/src/game/animatedgamemessage.cpp.

#include "gamemessage.h"

#include "menus/vertex.h"
#include "tools/array.h"

class AnimatedGameMessage : public GameMessage
{
public:
   AnimatedGameMessage();

   void setVertices(const Array<Vertex>& vertices);
   const Array<Vertex>& getVertices() const;

   float getAlpha() const;

   void initialize() override;

protected:
   float _alpha = 1.0f;
   Array<Vertex> _vertices;
};
