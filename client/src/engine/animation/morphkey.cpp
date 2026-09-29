#include "morphkey.h"

int32_t MorphKey::getVertexCount() const
{
   return _vertices.size();
}

const List<Vector>& MorphKey::getVertices() const
{
   return _vertices;
}

const List<Vector>& MorphKey::getNormals() const
{
   return _normals;
}

void MorphKey::load(Stream* stream)
{
   KeyBase::load(stream);
   _vertices << *stream;
}

void MorphKey::write(Stream* stream)
{
   KeyBase::write(stream);
   _vertices >> *stream;
}

void MorphKey::calculateNormals(const Array<uint16_t>& index_buffer)
{
   const int32_t vertex_count = _vertices.size();
   const int32_t index_count = index_buffer.size();
   _normals.init(vertex_count);
   for (int32_t i = 0; i < vertex_count; i++)
   {
      _normals[i].set(0.0f, 0.0f, 0.0f);
   }

   for (int32_t i = 0; i < index_count; i += 3)
   {
      const int32_t i1 = index_buffer[i];
      const int32_t i2 = index_buffer[i + 1];
      const int32_t i3 = index_buffer[i + 2];

      const Vector& v1 = _vertices[i1];
      const Vector& v2 = _vertices[i2];
      const Vector& v3 = _vertices[i3];

      Vector n = (v2 - v1) % (v3 - v1);
      n.normalize();

      _normals[i1] += n;
      _normals[i2] += n;
      _normals[i3] += n;
   }

   for (int32_t i = 0; i < vertex_count; i++)
   {
      _normals[i].normalize();
   }
}
