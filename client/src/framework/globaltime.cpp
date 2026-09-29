#include "globaltime.h"

GlobalTime* GlobalTime::_instance = nullptr;

GlobalTime::GlobalTime()
{
   _instance = this;
}

GlobalTime::~GlobalTime()
{
   _instance = nullptr;
}

GlobalTime* GlobalTime::Instance()
{
   return _instance;
}
