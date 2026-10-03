#pragma once

// GLES3 port of client/src/game/animatedgamemessage.cpp.

#include "gamemessage.h"

#include <vector>
#include "menus/vertex.h"

class AnimatedGameMessage : public GameMessage
{
public:
   AnimatedGameMessage();

   void setVertices(const std::vector<Vertex>& vertices);
   const std::vector<Vertex>& getVertices() const;

   float getAlpha() const;

   void initialize() override;

protected:
   float _alpha = 1.0f;
   std::vector<Vertex> _vertices;
};
