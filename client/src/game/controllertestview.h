#pragma once

#include "controllerinput.h"
#include "stickcalibration.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

class MenuPage;

/// \brief the controller picture of the controls options page (the joystick part of
/// GameMenuInterfaceOptions): name, held buttons, d-pad and stick of the controller used last,
/// plus the stick calibration that picks the steering axes.
class ControllerTestView
{
public:
   explicit ControllerTestView(ControllerInput& controller_input);
   ControllerTestView(const ControllerTestView&) = delete;
   ControllerTestView& operator=(const ControllerTestView&) = delete;
   ~ControllerTestView();

   /// \brief shows the view while the controls options page is current
   void onPageChanged(const std::string& page);

   /// \brief follows the controllers, call once per frame
   void update();

private:
   void reset();
   void selectDevice();
   void showName();
   void showButtons(const ControllerInput::State& state);
   void showDpad(const ControllerInput::State& state);
   void showStick(int16_t x, int16_t y);
   void calibrate(const ControllerInput::State& state);

   MenuPage& page() const;

   struct Position
   {
      int32_t x = 0;
      int32_t y = 0;
   };

   ControllerInput& _controller_input;
   bool _shown = false;

   std::optional<ControllerInput::Id> _device;
   std::unordered_map<ControllerInput::Id, ControllerInput::State> _states;
   StickCalibration _calibration;

   //! where the stick's layers rest, they're moved by its deflection
   std::optional<Position> _stick_active_position;
   std::optional<Position> _stick_outline_position;
};
