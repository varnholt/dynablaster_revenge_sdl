#pragma once

#include "tools/referenced.h"

#include <cstdint>

class Texture : public Referenced
{
public:
   Texture() = default;
   Texture(uint32_t texture_id);
   Texture(const Texture& texture);
   ~Texture() override;

   Texture& operator=(const Texture& texture);

   operator uint32_t() const
   {
      return _texture_id;
   }

   uint32_t getTexture() const
   {
      return _texture_id;
   }

private:
   uint32_t _texture_id = 0;
};
