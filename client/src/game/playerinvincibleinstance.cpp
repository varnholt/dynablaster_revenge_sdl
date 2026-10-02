#include "playerinvincibleinstance.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"

PlayerInvincibleInstance::PlayerInvincibleInstance()
{
   for (int i = 0; i < 2; i++)
   {
      glGenTextures(1, &_texture[i]);
      glBindTexture(GL_TEXTURE_2D, _texture[i]);
      if (i == 0)
      {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      }
      else
      {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      }
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_2D, 0);
   }

   for (int i = 0; i < 2; i++)
   {
      glGenFramebuffers(1, &_target[i]);
      glBindFramebuffer(GL_FRAMEBUFFER, _target[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture[i], 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer::screenTarget());
   }
}

PlayerInvincibleInstance::~PlayerInvincibleInstance()
{
   glDeleteFramebuffers(1, &_target[0]);
   glDeleteFramebuffers(1, &_target[1]);
   glDeleteTextures(1, &_texture[0]);
   glDeleteTextures(1, &_texture[1]);
}

void PlayerInvincibleInstance::remove()
{
   _remove = true;
}

void PlayerInvincibleInstance::setRemove(bool remove)
{
   _remove = remove;
}

bool PlayerInvincibleInstance::update(float dt)
{
   dt *= 0.025f;
   if (_remove)
   {
      if (_fade > 0.0f)
      {
         _fade -= dt;
         if (_fade <= 0.0f)
         {
            _fade = 0.0f;
            return false;
         }
      }
      else
      {
         return false;
      }
   }
   else
   {
      if (_fade < 1.0f)
      {
         _fade += dt;
         if (_fade > 1.0f)
         {
            _fade = 1.0f;
         }
      }
   }
   return true;
}

void PlayerInvincibleInstance::setMaterial(Material* mat)
{
   _material = mat;
}

Material* PlayerInvincibleInstance::getMaterial() const
{
   return _material;
}

int PlayerInvincibleInstance::width() const
{
   return _width;
}

int PlayerInvincibleInstance::height() const
{
   return _height;
}

uint32_t PlayerInvincibleInstance::texture(int id) const
{
   return _texture[id];
}

void PlayerInvincibleInstance::bind(int id)
{
   glBindFramebuffer(GL_FRAMEBUFFER, _target[id]);
   glViewport(0, 0, _width, _height);
}

void PlayerInvincibleInstance::unbind()
{
   glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer::screenTarget());
}

void PlayerInvincibleInstance::setCenter(const Vector& center)
{
   _center = center;
}

const Vector& PlayerInvincibleInstance::getCenter() const
{
   return _center;
}

void PlayerInvincibleInstance::setRect(const Vector& min, const Vector& max)
{
   _min = min;
   _max = max;
}

const Vector& PlayerInvincibleInstance::min2d() const
{
   return _min;
}

const Vector& PlayerInvincibleInstance::max2d() const
{
   return _max;
}

float PlayerInvincibleInstance::getFade() const
{
   return _fade;
}

void PlayerInvincibleInstance::setFade(float fade)
{
   _fade = fade;
}
