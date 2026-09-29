// GLES3 port of client/src/game/animatedgamemessage.cpp.

#include "animatedgamemessage.h"

AnimatedGameMessage::AnimatedGameMessage() : GameMessage()
{
}

void AnimatedGameMessage::setVertices(const Array<Vertex>& vertices)
{
   _vertices.copy(vertices);
}

const Array<Vertex>& AnimatedGameMessage::getVertices() const
{
   return _vertices;
}

float AnimatedGameMessage::getAlpha() const
{
   return _alpha;
}

void AnimatedGameMessage::initialize()
{
   GameMessage::initialize();
}
