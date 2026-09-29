#include "shape.h"
#include "tools/stream.h"

Shape::Shape(Node* parent) : Node(Node::idShape, parent)
{
}

void Shape::load(Stream* stream)
{
   Node::load(stream);

   const int32_t poly_count = stream->getInt();
   _polys.clear();
   _polys.reserve(poly_count);
   for (int32_t i = 0; i < poly_count; i++)
   {
      Chunk poly_chunk(stream);
      auto poly = std::make_unique<PolyLine>();
      poly->load(&poly_chunk);
      _polys.push_back(std::move(poly));
      poly_chunk.skip();
   }

   // read tracks
   Chunk animation(stream);
   _position_track.load(&animation);
   _rotation_track.load(&animation);
   _scale_track.load(&animation);
   _visibility_track.load(&animation);
   _flip_track.load(&animation);
   animation.skip();
}

void Shape::write(Stream* stream)
{
   Node::write(stream);

   // TODO: write poly lines

   // write tracks
   Chunk animation(stream, 2000, "Animation");
   _position_track.write(&animation);
   _rotation_track.write(&animation);
   _scale_track.write(&animation);
   _visibility_track.write(&animation);
   _flip_track.write(&animation);
}

int32_t Shape::getPolyCount() const
{
   return static_cast<int32_t>(_polys.size());
}

Shape::PolyLine* Shape::getPoly(int32_t index) const
{
   return _polys[index].get();
}
