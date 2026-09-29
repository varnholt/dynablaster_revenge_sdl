#pragma once

#include <cstdint>

// intrusive, shared reference counter for the copy-on-write containers (Array, String, Image, ...)
class Referenced
{
public:
   Referenced();
   Referenced(const Referenced& other);
   Referenced(const Referenced* other);
   virtual ~Referenced();

   void addRef() const;  // add reference
   bool deref();         // remove reference
   bool copyRef();       // copy reference if required

   int32_t getRefCount() const;  // get number of referencing objects
   int32_t* getRef() const;      // get reference pointer

protected:
   // kept as mReferences: derived classes outside tools/ access it directly
   int32_t* mReferences = nullptr;
};
