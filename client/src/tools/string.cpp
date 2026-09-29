#include "string.h"

#include <algorithm>
#include <cstring>

#include "stream.h"

String::String() = default;

String::String(const char* text)
{
   if (text)
   {
      const auto size = static_cast<int32_t>(std::strlen(text));
      alloc(size);
      std::memcpy(_data, text, size + 1);
   }
}

String::String(const String& other) : Referenced(other), _data(const_cast<char*>(other.data()))
{
}

// delete string if no more references exist
String::~String()
{
   if (getRefCount() == 1)
   {
      dealloc();
   }
}

String String::mid(int32_t start, int32_t end) const
{
   start = std::clamp(start, 0, size());
   end = std::clamp(end, 0, size());

   String result;
   result.alloc(end - start);
   char* destination = result._data;
   std::memcpy(destination, data() + start, end - start);
   destination[end - start] = 0;

   return result;
}

bool String::isEmpty() const
{
   return (size() == 0);
}

int32_t String::indexOf(const String& other) const
{
   const int32_t size1 = size();
   const int32_t size2 = other.size();
   if (size2 <= size1)
   {
      for (int32_t index = 0; index < size1 - size2; index++)
      {
         int32_t i = 0;
         while (i < size2 && _data[index + i] == other[i])
         {
            i++;
         }
         if (i == size2)
         {
            return index;
         }
      }
   }
   return -1;
}

// allocate data for "size" characters + \0
void String::alloc(int32_t size)
{
   // single chunk to store size, text and \0
   _data = new char[size + sizeof(int32_t) + 1];
   *reinterpret_cast<int32_t*>(_data) = size;
   // "_data" always points to the text
   _data += sizeof(int32_t);
}

void String::dealloc()
{
   delete[] dataIntern();
   _data = nullptr;
}

void String::setSize(int32_t size)
{
   auto* data = reinterpret_cast<int32_t*>(dataIntern());
   if (data)
   {
      *data = size;
   }
}

// originally allocated data (including the size field)
char* String::dataIntern() const
{
   if (_data)
   {
      return _data - sizeof(int32_t);
   }
   return nullptr;
}

String& String::operator<<(Stream& stream)
{
   load(&stream);
   return *this;
}

String::operator const char*() const
{
   return _data;
}

char String::operator[](int32_t index) const
{
   return _data[index];
}

String& String::operator=(const String& other)
{
   if (this != &other)
   {
      // string is not referenced: delete data and reference counter
      if (getRefCount() == 1)
      {
         dealloc();
         delete mReferences;
      }

      mReferences = other.getRef();
      addRef();
      _data = const_cast<char*>(other.data());
   }
   return *this;
}

void String::clear()
{
   // if string was referenced we have to leave the existing data untouched
   if (!copyRef())
   {
      dealloc();
   }
   else
   {
      _data = nullptr;
   }
}

char String::get(int32_t index) const
{
   return _data[index];
}

void String::append(const String& other)
{
   char* old_data = dataIntern();
   const char* data = _data;

   const bool need_delete = !copyRef();

   const int32_t size1 = size();
   const int32_t size2 = other.size();
   alloc(size1 + size2);
   if (size1 > 0)
   {
      std::memcpy(_data, data, size1);
   }
   if (other.data())
   {
      std::memcpy(_data + size1, other.data(), size2 + 1);
   }
   else
   {
      _data[size1] = 0;
   }

   if (need_delete)
   {
      delete[] old_data;
   }
}

void String::operator+=(const String& other)
{
   append(other);
}

String String::operator+(const String& other) const
{
   String result = *this;
   result.append(other);
   return result;
}

bool String::operator==(const String& other) const
{
   if (size() != other.size())
   {
      return false;
   }
   const int32_t length = size();
   for (int32_t i = 0; i < length; i++)
   {
      if (_data[i] != other[i])
      {
         return false;
      }
   }
   return true;
}

bool String::operator==(const char* other) const
{
   if (other == nullptr)
   {
      return size() == 0;
   }

   const int32_t length = size();
   for (int32_t i = 0; i < length; i++)
   {
      if (_data[i] != other[i])
      {
         return false;
      }
   }
   return true;
}

bool String::operator!=(const String& other) const
{
   return !(*this == other);
}

bool String::operator<(const String& other) const
{
   const char* data = other.data();

   // at least one of both is empty
   if (!_data || !data)
   {
      if (!_data && !data)
      {
         return false;
      }
      return (!_data);
   }

   const int32_t length = std::min(size(), other.size());
   for (int32_t i = 0; i <= length; i++)
   {
      if (_data[i] < data[i])
      {
         return true;
      }
      if (_data[i] > data[i])
      {
         return false;
      }
   }

   return false;
}

bool String::operator>(const String& other) const
{
   return other < *this;
}

const char* String::data() const
{
   return _data;
}

int32_t String::size() const
{
   const auto* size = reinterpret_cast<const int32_t*>(dataIntern());
   if (size)
   {
      return *size;
   }
   return 0;
}

void String::load(Stream* stream)
{
   if (!copyRef())
   {
      dealloc();
   }

   int32_t size = stream->getByte();
   alloc(size);

   stream->getData(_data, size);
   _data[size] = 0;

   // was \0 already present?
   while (size > 0 && _data[size - 1] == 0)
   {
      size--;
   }
   setSize(size);
}

void String::write(Stream* stream)
{
   const int32_t length = std::min(size(), 255);
   stream->writeByte(static_cast<uint8_t>(length));
   stream->writeData(_data, length);
}
