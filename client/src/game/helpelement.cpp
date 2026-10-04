#include "helpelement.h"

#include <utility>

HelpElement::HelpElement(std::string page, std::string message, Constants::HelpSeverity severity, Constants::HelpLocation location, int delay)
    : _page(std::move(page)), _message(std::move(message)), _severity(severity), _location(location), _delay(delay)
{
}

void HelpElement::start()
{
   _start_time.start();
   _started = true;
}

bool HelpElement::isStarted() const
{
   return _started;
}

bool HelpElement::isFinished() const
{
   return _started && getElapsed() >= display_duration;
}

void HelpElement::startDelay()
{
   _delay_time.start();
   _delay_started = true;
}

bool HelpElement::isDelayed() const
{
   return _delay > 0;
}

bool HelpElement::isDelayStarted() const
{
   return _delay_started;
}

bool HelpElement::isDelayElapsed() const
{
   return !isDelayed() || (_delay_started && _delay_time.elapsed() >= static_cast<float>(_delay));
}

const std::string& HelpElement::getPage() const
{
   return _page;
}

const std::string& HelpElement::getMessage() const
{
   return _message;
}

Constants::HelpSeverity HelpElement::getSeverity() const
{
   return _severity;
}

Constants::HelpLocation HelpElement::getLocation() const
{
   return _location;
}

float HelpElement::getElapsed() const
{
   return _start_time.elapsed();
}
