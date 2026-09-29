#include "texture.h"
#include "texturepool.h"

Texture::Texture(uint32_t texture_id) : _texture_id(texture_id)
{
}

Texture::Texture(const Texture& texture) : Referenced(texture), _texture_id(texture.getTexture())
{
}

Texture::~Texture()
{
   // last instance outside the pool (the pool holds another one)
   if (_texture_id && getRefCount() <= 2)
   {
      TexturePool::Instance()->remove(*this);
   }
}

Texture& Texture::operator=(const Texture& texture)
{
   if (this != &texture)
   {
      // texture is not referenced: queue for deletion
      if (getRefCount() == 1)
      {
         if (_texture_id)
         {
            TexturePool::Instance()->remove(_texture_id);
         }
         delete _references;
      }

      _references = texture.getRef();
      addRef();
      _texture_id = texture.getTexture();
   }
   return *this;
}
