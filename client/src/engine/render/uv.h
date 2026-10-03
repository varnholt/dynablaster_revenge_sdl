#pragma once

// do not derive from Streamable

class Stream;

class UV
{
public:
   UV() = default;
   UV(float u, float v);

   void operator<<(Stream& stream);
   void operator>>(Stream& stream);

   void load(Stream& stream);
   void write(Stream& stream);

   float u = 0.0f;
   float v = 0.0f;
};
