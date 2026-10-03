#include "morphtrack.h"

MorphTrack::MorphTrack() : Track<MorphKey>(Track::idVertexMorph)
{
}

void MorphTrack::get(std::vector<Vector>& vertices, std::vector<Vector>& normals, float time)
{
   const MorphKey& first = key(0);
   const MorphKey& last = key(size() - 1);

   const int32_t count = first.getVertexCount();

   if (time <= first.time() || time >= last.time())
   {
      const MorphKey& source = (time <= first.time()) ? first : last;
      const std::vector<Vector>& source_vertices = source.getVertices();
      const std::vector<Vector>& source_normals = source.getNormals();
      for (int32_t i = 0; i < count; i++)
      {
         vertices[i] = source_vertices[i];
         normals[i] = source_normals[i];
      }
   }
   else
   {
      const float f = interpolate(time);

      const std::vector<Vector>& v1 = prevKey().getVertices();
      const std::vector<Vector>& n1 = prevKey().getNormals();

      const std::vector<Vector>& v2 = nextKey().getVertices();
      const std::vector<Vector>& n2 = nextKey().getNormals();

      for (int32_t i = 0; i < count; i++)
      {
         vertices[i] = v1[i] + (v2[i] - v1[i]) * f;
         normals[i] = n1[i] + (n2[i] - n1[i]) * f;
      }
   }
}

void MorphTrack::calculateNormals(const std::vector<uint16_t>& indices)
{
   for (MorphKey& morph_key : _keys)
   {
      morph_key.calculateNormals(indices);
   }
}
