#include "bone.h"
#include "../nodes/scenegraph.h"

#include <array>
#include <cstdio>

Bone::Bone(const Bone& bone, Array<int32_t>* remap) : _id(bone.id()), _init_transform(bone.transform())
{
   const List<Weight>& weights = bone.weightList();
   for (int32_t w = 0; w < weights.size(); w++)
   {
      const Weight& weight = weights[w];
      const Array<int32_t>& list = remap[weight.id()];
      for (int32_t i = 0; i < list.size(); i++)
      {
         _weights.add(Weight(list[i], weight.weight()));
      }
   }
}

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
   return _weights.size();
}

const Matrix& Bone::transform() const
{
   return _init_transform;
}

Weight* Bone::weights() const
{
   return _weights.data();
}

const List<Weight>& Bone::weightList() const
{
   return _weights;
}

void Bone::load(Stream* stream)
{
   _id = stream->getInt();
   _init_transform.load(stream);
   _weights.load(stream);

   constexpr int32_t max_vertices = 10000;
   std::array<int32_t, max_vertices> weights_per_vertex{};

   for (int32_t i = 0; i < _weights.size(); i++)
   {
      const Weight& weight = _weights[i];
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

   SceneGraph* scene = SceneGraph::instance();
   if (scene)
   {
      _id += scene->getNodeStartIndex();
   }
}

void Bone::write(Stream* stream)
{
   stream->writeInt(_id);
   _init_transform.write(stream);

   _weights.write(stream);
}
