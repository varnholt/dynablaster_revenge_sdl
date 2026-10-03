#include "texture.h"
#include "texturepool.h"

Texture::Texture(uint32_t texture_id) : _texture_id(texture_id), _handle(std::make_shared<const uint32_t>(texture_id))
{
}

Texture::~Texture()
{
   // last instance outside the pool (the pool holds another one)
   if (_texture_id && getRefCount() <= 2)
   {
      TexturePool::Instance().remove(*this);
   }
}

Texture& Texture::operator=(const Texture& texture)
{
   if (this != &texture)
   {
      // texture is not referenced: queue for deletion
      if (getRefCount() == 1 && _texture_id)
      {
         TexturePool::Instance().remove(Texture(_texture_id));
      }

      _handle = texture._handle;
      _texture_id = texture.getTexture();
   }
   return *this;
}

int32_t Texture::getRefCount() const
{
   return _handle ? static_cast<int32_t>(_handle.use_count()) : 1;
}
