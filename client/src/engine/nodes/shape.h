// shape object
// contains a number of polylines

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "node.h"
#include "tools/list.h"

class Stream;

class Shape : public Node
{
public:
   class PolyLine
   {
   public:
      bool closed() const
      {
         return (_flags & 1);
      }

      void load(Stream* stream)
      {
         _flags = stream->getInt();

         // vertex types are not used
         int32_t count = stream->getInt();
         while (count--)
         {
            stream->getInt();
         }

         _vertices.load(stream);
      }

      int32_t getVertexCount() const
      {
         return _vertices.size();
      }

      const Vector& getVertex(int32_t index) const
      {
         return _vertices[index];
      }

   private:
      int32_t _flags = 0;
      List<Vector> _vertices;
   };

   Shape(Node* parent = nullptr);
   void load(Stream* stream) override;
   void write(Stream* stream) override;

   int32_t getPolyCount() const;
   PolyLine* getPoly(int32_t index) const;

private:
   std::vector<std::unique_ptr<PolyLine>> _polys;
};
