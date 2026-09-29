#include "referenced.h"

Referenced::Referenced() : _references(new int32_t(1))
{
}

Referenced::Referenced(const Referenced& other) : _references(other.getRef())
{
   addRef();
}

Referenced::Referenced(const Referenced* other) : _references(other->getRef())
{
   addRef();
}

Referenced::~Referenced()
{
   if (!deref())
   {
      delete _references;
      _references = nullptr;
   }
}

void Referenced::addRef() const
{
   (*_references)++;
}

bool Referenced::deref()
{
   (*_references)--;
   return (*_references != 0);
}

bool Referenced::copyRef()
{
   if (*_references > 1)
   {
      (*_references)--;
      _references = new int32_t(1);
      return true;
   }

   return false;
}

int32_t Referenced::getRefCount() const
{
   return *_references;
}

int32_t* Referenced::getRef() const
{
   return _references;
}
