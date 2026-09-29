#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

class LensFlare;

// owns every flare setup from flares.ini and draws the one the current level activated
class LensFlareFactory
{
public:
   LensFlareFactory();
   ~LensFlareFactory();

   LensFlareFactory(const LensFlareFactory&) = delete;
   LensFlareFactory& operator=(const LensFlareFactory&) = delete;

   //! \return \c true if a flare with that key exists; an empty key deactivates
   bool activate(const std::string& key);

   //! draws on top of the frame, call after the scene with the scene camera still set
   void draw();

private:
   std::map<std::string, std::unique_ptr<LensFlare>> _lens_flares;
   LensFlare* _active = nullptr;

   uint32_t _shader = 0;
   int32_t _time_param = -1;
   int32_t _sun_param = -1;
   int32_t _length_param = -1;
   int32_t _texture_param = -1;
};
