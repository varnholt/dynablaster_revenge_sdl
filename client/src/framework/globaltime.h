// global time interface
// whoever inherits this is responsible to make "getTime" return a constant time for each frame

#pragma once

class GlobalTime
{
public:
   GlobalTime();
   virtual ~GlobalTime();

   GlobalTime(const GlobalTime&) = delete;
   GlobalTime& operator=(const GlobalTime&) = delete;

   // the most recently constructed time source, only valid while one exists
   static GlobalTime& Instance();

   virtual float getTime() const = 0;
};
