#pragma once

#include <bit>
#include <cstdint>
#include <string>

class String;

class Stream
{
public:
   Stream() = default;
   virtual ~Stream() = default;

   virtual const String& getPath() const;

   int32_t getEndian() const;           // get current endian mode
   void setEndian(int32_t big_endian);  // set endian mode

   virtual void getData(void* destination, int32_t size) = 0;  // get "size" bytes and store in "destination"
   virtual void writeData(void* source, int32_t size) = 0;     // write "size" bytes

   virtual char getChar();           // get single character
   virtual uint8_t getByte();        // get single byte
   virtual int32_t getWord();        // get unsigned word (2 byte)
   virtual int16_t getShort();       // get short (2 byte)
   virtual int32_t getInt();         // get integer (4 byte)
   virtual float getFloat();         // get float (4 byte)
   virtual std::string getString();  // get string (0 terminated)

   virtual void writeChar(char);           // write single character
   virtual void writeByte(uint8_t);        // write single byte
   virtual void writeWord(uint16_t);       // write word (2 byte)
   virtual void writeShort(int16_t);       // write short
   virtual void writeInt(int32_t);         // write integer (4 byte)
   virtual void writeFloat(float);         // write float (4 byte)
   virtual void writeString(const char*);  // write string (0 terminated)

   virtual int32_t pos() const;
   virtual void skip(int32_t size);

private:
   static constexpr int32_t kMachineEndian = (std::endian::native == std::endian::big) ? 1 : 0;

   void swap32(void* data);
   void swap16(void* data);

   int32_t _endian = 0;

protected:
   int32_t _position = 0;
};

Stream& operator<<(Stream& stream, float&);
void operator<<(float&, Stream& stream);
Stream& operator<<(Stream& stream, int32_t&);

void operator>>(float&, Stream& stream);
