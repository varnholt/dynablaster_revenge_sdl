#include "morphtrack.h"

MorphTrack::MorphTrack() : Track<MorphKey>(Track::idVertexMorph)
{
}

void MorphTrack::get(List<Vector>& vertices, List<Vector>& normals, float time)
{
   const MorphKey& first = Track<MorphKey>::get(0);
   const MorphKey& last = Track<MorphKey>::getLast();

   const int32_t count = first.getVertexCount();

   if (time <= first.time() || time >= last.time())
   {
      const MorphKey& source = (time <= first.time()) ? first : last;
      const List<Vector>& source_vertices = source.getVertices();
      const List<Vector>& source_normals = source.getNormals();
      for (int32_t i = 0; i < count; i++)
      {
         vertices[i] = source_vertices[i];
         normals[i] = source_normals[i];
      }
   }
   else
   {
      const float f = interpolate(time);

      const List<Vector>& v1 = prevKey().getVertices();
      const List<Vector>& n1 = prevKey().getNormals();

      const List<Vector>& v2 = nextKey().getVertices();
      const List<Vector>& n2 = nextKey().getNormals();

      for (int32_t i = 0; i < count; i++)
      {
         vertices[i] = v1[i] + (v2[i] - v1[i]) * f;
         normals[i] = n1[i] + (n2[i] - n1[i]) * f;
      }
   }
}

void MorphTrack::calculateNormals(const Array<uint16_t>& indices)
{
   for (int32_t i = 0; i < size(); i++)
   {
      key(i).calculateNormals(indices);
   }
}
