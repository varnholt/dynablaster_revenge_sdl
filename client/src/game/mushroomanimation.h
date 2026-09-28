#pragma once

// fades the shroom screen filter in on start() and out on abort()
class MushroomAnimation
{
public:
   void start();
   void abort();
   void update();

   bool isActive() const;
   float getIntensity() const;

private:
   float _time = 0.0f;
   float _start_time = 0.0f;
   float _intensity = 0.0f;
   bool _active = false;
   bool _aborted = false;
};
