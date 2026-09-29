#include "keybase.h"
#include "tools/stream.h"

KeyBase::KeyBase(int32_t time) : _time(time)
{
}

int32_t KeyBase::time() const
{
   return _time;
}

void KeyBase::load(Stream* stream)
{
   _time = stream->getInt();
}

void KeyBase::write(Stream* stream)
{
   stream->writeInt(_time);
}
