#include "bone.h"
#include "tools/stream.h"

#include <array>
#include <cstdio>

int32_t Bone::id() const
{
   return _id;
}

void Bone::setId(int32_t id)
{
   _id = id;
}

int32_t Bone::count() const
{
   return static_cast<int32_t>(_weights.size());
}

const Matrix& Bone::transform() const
{
   return _init_transform;
}

std::span<const Weight> Bone::weights() const
{
   return _weights;
}

const std::vector<Weight>& Bone::weightList() const
{
   return _weights;
}

void Bone::load(Stream& stream)
{
   _id = stream.getInt();
   _init_transform.load(stream);
   loadList(stream, _weights);

   constexpr int32_t max_vertices = 10000;
   std::array<int32_t, max_vertices> weights_per_vertex{};

   for (const Weight& weight : _weights)
   {
      if (weight.id() < max_vertices)
      {
         weights_per_vertex[weight.id()]++;
      }
   }

   for (int32_t i = 0; i < max_vertices; i++)
   {
      if (weights_per_vertex[i] > 3)
      {
         std::printf("vertex %d has %d weights \n", i, weights_per_vertex[i]);
      }
   }
}

void Bone::write(Stream& stream)
{
   stream.writeInt(_id);
   _init_transform.write(stream);

   writeList(stream, _weights);
}
