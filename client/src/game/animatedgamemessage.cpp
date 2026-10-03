// GLES3 port of client/src/game/animatedgamemessage.cpp.

#include "animatedgamemessage.h"

AnimatedGameMessage::AnimatedGameMessage() : GameMessage()
{
}

void AnimatedGameMessage::setVertices(const std::vector<Vertex>& vertices)
{
   _vertices = vertices;
}

const std::vector<Vertex>& AnimatedGameMessage::getVertices() const
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
