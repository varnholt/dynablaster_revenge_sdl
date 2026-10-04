#pragma once

#include "constants.h"
#include "framework/frametimer.h"

#include <string>

/// \brief one queued toast of GameHelpDrawable: fades in, stays, fades out; optionally waits
/// \a delay ms on its page before it starts
class HelpElement
{
public:
   HelpElement(std::string page, std::string message, Constants::HelpSeverity severity, Constants::HelpLocation location, int delay = 0);

   void start();
   bool isStarted() const;
   bool isFinished() const;

   void startDelay();
   bool isDelayed() const;
   bool isDelayStarted() const;
   bool isDelayElapsed() const;

   const std::string& getPage() const;
   const std::string& getMessage() const;
   Constants::HelpSeverity getSeverity() const;
   Constants::HelpLocation getLocation() const;

   //! ms since start()
   float getElapsed() const;

   static constexpr float fade_in_duration = 1000.0f;
   static constexpr float idle_duration = 6000.0f;
   static constexpr float fade_out_duration = 1000.0f;
   static constexpr float popup_duration = 2000.0f;
   static constexpr float display_duration = fade_in_duration + idle_duration + fade_out_duration;

private:
   std::string _page;
   std::string _message;
   Constants::HelpSeverity _severity;
   Constants::HelpLocation _location;
   int _delay = 0;

   FrameTimer _start_time;
   FrameTimer _delay_time;
   bool _started = false;
   bool _delay_started = false;
};
