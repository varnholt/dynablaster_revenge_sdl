#pragma once

#include <cstdint>
#include <memory>

// shared handle of a pooled GL texture; the last handle outside the pool queues the texture for deletion
class Texture
{
public:
   Texture() = default;
   Texture(uint32_t texture_id);
   Texture(const Texture& texture) = default;
   ~Texture();

   Texture& operator=(const Texture& texture);

   operator uint32_t() const
   {
      return _texture_id;
   }

   uint32_t getTexture() const
   {
      return _texture_id;
   }

   // number of handles sharing this texture (including this one)
   int32_t getRefCount() const;

private:
   uint32_t _texture_id = 0;
   std::shared_ptr<const uint32_t> _handle;
};
