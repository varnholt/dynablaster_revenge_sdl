#include "referenced.h"

Referenced::Referenced() : mReferences(new int32_t(1))
{
}

Referenced::Referenced(const Referenced& other) : mReferences(other.getRef())
{
   addRef();
}

Referenced::Referenced(const Referenced* other) : mReferences(other->getRef())
{
   addRef();
}

Referenced::~Referenced()
{
   if (!deref())
   {
      delete mReferences;
      mReferences = nullptr;
   }
}

void Referenced::addRef() const
{
   (*mReferences)++;
}

bool Referenced::deref()
{
   (*mReferences)--;
   return (*mReferences != 0);
}

bool Referenced::copyRef()
{
   if (*mReferences > 1)
   {
      (*mReferences)--;
      mReferences = new int32_t(1);
      return true;
   }

   return false;
}

int32_t Referenced::getRefCount() const
{
   return *mReferences;
}

int32_t* Referenced::getRef() const
{
   return mReferences;
}
