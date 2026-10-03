#include "morphkey.h"

int32_t MorphKey::getVertexCount() const
{
   return static_cast<int32_t>(_vertices.size());
}

const std::vector<Vector>& MorphKey::getVertices() const
{
   return _vertices;
}

const std::vector<Vector>& MorphKey::getNormals() const
{
   return _normals;
}

void MorphKey::load(Stream& stream)
{
   KeyBase::load(stream);
   loadList(stream, _vertices);
}

void MorphKey::write(Stream& stream)
{
   KeyBase::write(stream);
   writeList(stream, _vertices);
}

void MorphKey::calculateNormals(const std::vector<uint16_t>& index_buffer)
{
   const auto index_count = index_buffer.size();
   _normals.assign(_vertices.size(), Vector(0.0f, 0.0f, 0.0f));

   for (size_t i = 0; i + 2 < index_count; i += 3)
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

   for (Vector& normal : _normals)
   {
      normal.normalize();
   }
}
