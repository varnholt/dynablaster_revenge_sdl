#include "dummy.h"
#include "tools/stream.h"

Dummy::Dummy(Node* parent) : Node(Node::idDummy, parent)
{
}

void Dummy::load(Stream* stream)
{
   Node::load(stream);

   // read tracks
   Chunk animation(stream);
   _position_track.load(&animation);
   _rotation_track.load(&animation);
   _scale_track.load(&animation);
   _visibility_track.load(&animation);
   _flip_track.load(&animation);
   animation.skip();
}

void Dummy::write(Stream* stream)
{
   Node::write(stream);

   // write tracks
   Chunk animation(stream, 2000, "Animation");
   _position_track.write(&animation);
   _rotation_track.write(&animation);
   _scale_track.write(&animation);
   _visibility_track.write(&animation);
   _flip_track.write(&animation);
}
