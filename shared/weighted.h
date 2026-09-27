#pragma once

template<typename O, typename W>
class Weighted
{

public:

   Weighted() = default;

   Weighted(O object, W weight)
    : _object(object),
      _weight(weight)
   {

   }

   [[nodiscard]] bool operator == (const Weighted& other) const
   {
      return other._weight == _weight;
   }

   [[nodiscard]] bool operator < (const Weighted& other) const
   {
      return other._weight < _weight;
   }

   [[nodiscard]] O getObject() const
   {
      return _object;
   }

   [[nodiscard]] W getWeight() const
   {
      return _weight;
   }

   void setWeight(W weight)
   {
       _weight = weight;
   }


protected:

   O _object;
   W _weight;
};

