// string class
// holds a single \0-terminated character string, shared copy-on-write between copies

#pragma once

#include <cstdint>
#include <cstring>

#include "referenced.h"
#include "streamable.h"

class String : public Referenced, public Streamable
{
public:
   String();
   String(const char* text);
   String(const String& other);  // references "other"
   ~String() override;

   String& operator=(const String& other);  // references "other"

   operator const char*() const;

   char operator[](int32_t index) const;

   String operator+(const String& other) const;
   void operator+=(const String& other);

   bool operator==(const String& other) const;
   bool operator==(const char* other) const;
   bool operator!=(const String& other) const;
   bool operator<(const String& other) const;
   bool operator>(const String& other) const;

   String& operator<<(Stream& stream);

   bool isEmpty() const;
   void clear();
   void append(const String& other);

   // index of substring "other", -1 if not found
   int32_t indexOf(const String& other) const;

   // characters [start, end)
   String mid(int32_t start, int32_t end) const;

   char get(int32_t index) const;
   const char* data() const;
   int32_t size() const;

   void load(Stream* stream) override;
   void write(Stream* stream) override;

private:
   void alloc(int32_t size);
   void dealloc();
   char* dataIntern() const;
   void setSize(int32_t size);

   // text, preceded by an int32_t length field in the same allocation (null for empty strings)
   char* _data = nullptr;
};
