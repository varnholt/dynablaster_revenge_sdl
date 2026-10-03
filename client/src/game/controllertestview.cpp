#include "controllertestview.h"

#include "gamesettings.h"

#include "menus/menu.h"
#include "menus/menupage.h"
#include "menus/menupageitem.h"
#include "menus/menupagelabelitem.h"

#include "logging.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace
{

const std::string kOptionsControls = "data/menus/options_controls.psd";
const std::string kName = "label_controller_name";
const std::string kStickActive = "controller_analog_1_active";
const std::string kStickOutline = "controller_analog_1_outline";
const std::string kSecondStickActive = "controller_analog_2_active";

//! the stick layers move this many page pixels at full deflection
constexpr float kStickRange = 10.0f;

//! the name label holds this many characters
constexpr size_t kNameLength = 16;

const std::array<std::pair<SDL_GamepadButton, std::string>, 8> kButtonLabels{{
   {SDL_GAMEPAD_BUTTON_SOUTH, "label_controller_button_a"},
   {SDL_GAMEPAD_BUTTON_EAST, "label_controller_button_b"},
   {SDL_GAMEPAD_BUTTON_WEST, "label_controller_button_x"},
   {SDL_GAMEPAD_BUTTON_NORTH, "label_controller_button_y"},
   {SDL_GAMEPAD_BUTTON_BACK, "label_controller_button_back"},
   {SDL_GAMEPAD_BUTTON_START, "label_controller_button_start"},
   {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "label_controller_button_shoulder_l"},
   {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "label_controller_button_shoulder_r"},
}};

//! indexed by 1 + x + 3 * (1 + y) with x, y in -1..1, y pointing down
const std::array<std::string, 9> kDpadLabels{
   "label_controller_top_left",
   "label_controller_top",
   "label_controller_top_right",
   "label_controller_left",
   "label_controller_center",
   "label_controller_right",
   "label_controller_bottom_left",
   "label_controller_bottom",
   "label_controller_bottom_right",
};

bool isDpad(int32_t button)
{
   return button == SDL_GAMEPAD_BUTTON_DPAD_UP || button == SDL_GAMEPAD_BUTTON_DPAD_DOWN || button == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
          button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
}

}  // namespace

ControllerTestView::ControllerTestView(ControllerInput& controller_input) : _controller_input(controller_input)
{
}

ControllerTestView::~ControllerTestView() = default;

MenuPage& ControllerTestView::page() const
{
   return Menu::getInstance().getPageByName(kOptionsControls)->get();
}

void ControllerTestView::onPageChanged(const std::string& page_name)
{
   const bool show = page_name == kOptionsControls;
   if (show == _shown)
   {
      return;
   }

   _shown = show;
   _device.reset();
   _states.clear();
   _calibration.reset();

   // matches initializeButtonLabels()/resetJoystickStates(), leaving puts the stick back too
   reset();

   if (_shown)
   {
      selectDevice();
      showName();
   }
}

void ControllerTestView::reset()
{
   // the second stick isn't shown, only its outline
   if (const auto item = page().getPageItem(kSecondStickActive))
   {
      item->get().setVisible(false);
   }

   for (const auto& [button, label] : kButtonLabels)
   {
      if (const auto item = page().getPageItem(label))
      {
         item->get().setVisible(false);
      }
   }

   showDpad(ControllerInput::State{});
   showStick(0, 0);
}

void ControllerTestView::update()
{
   if (!_shown)
   {
      return;
   }

   const auto previous = _device;
   selectDevice();
   if (_device != previous)
   {
      showName();
   }

   const auto state = _device ? _controller_input.getState(*_device) : std::nullopt;
   if (!state)
   {
      reset();
      return;
   }

   showButtons(*state);
   showDpad(*state);

   const auto& controls = GameSettings::getInstance().getControllerSettings();
   const auto axis = [](int value) { return static_cast<size_t>(std::clamp(value, 0, SDL_GAMEPAD_AXIS_COUNT - 1)); };
   showStick(state->axes[axis(controls.getAnalogueAxis1())], state->axes[axis(controls.getAnalogueAxis2())]);

   calibrate(*state);
}

void ControllerTestView::selectDevice()
{
   // the controller whose state changed last, the first one to begin with
   const auto devices = _controller_input.getDevices();

   for (const auto& device : devices)
   {
      const auto state = _controller_input.getState(device.id);
      if (!state)
      {
         continue;
      }

      const auto it = _states.find(device.id);
      if (it != _states.end() && it->second != *state)
      {
         _device = device.id;
      }

      _states[device.id] = *state;
   }

   const bool connected =
      _device && std::ranges::any_of(devices, [this](const ControllerInput::DeviceInfo& device) { return device.id == *_device; });

   if (!connected)
   {
      _device = devices.empty() ? std::nullopt : std::optional{devices.front().id};
   }
}

void ControllerTestView::showName()
{
   const auto label_item = page().getPageItem<MenuPageLabelItem>(kName);
   if (!label_item)
   {
      return;
   }

   MenuPageLabelItem& label = *label_item;

   std::string name;
   for (const auto& device : _controller_input.getDevices())
   {
      if (_device && device.id == *_device)
      {
         name = device.name;
      }
   }

   // like displayJoystickName(), shortened to what the label holds
   if (name.length() > kNameLength)
   {
      name = name.substr(0, kNameLength - 3);
      name.erase(name.find_last_not_of(' ') + 1);
      name += "...";
   }

   label.setText(name);
   if (const auto layer = label.getCurrentLayer())
   {
      label.setCenterWidth(static_cast<float>(layer->get().getWidth()));
   }
}

void ControllerTestView::showButtons(const ControllerInput::State& state)
{
   // like processJoystickButton(): a button without a picture lights them all
   bool all = false;
   for (int32_t button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
   {
      const bool pictured = std::ranges::any_of(kButtonLabels, [button](const auto& entry) { return entry.first == button; });
      if (state.buttons[button] && !pictured && !isDpad(button))
      {
         all = true;
      }
   }

   for (const auto& [button, label] : kButtonLabels)
   {
      if (const auto item = page().getPageItem(label))
      {
         item->get().setVisible(all || state.buttons[button]);
      }
   }
}

void ControllerTestView::showDpad(const ControllerInput::State& state)
{
   const int32_t x = (state.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] ? 1 : 0) - (state.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] ? 1 : 0);
   const int32_t y = (state.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] ? 1 : 0) - (state.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] ? 1 : 0);
   const size_t pressed = static_cast<size_t>(1 + x + 3 * (1 + y));

   // exactly one of the nine is shown, like processJoystickHat()
   for (size_t i = 0; i < kDpadLabels.size(); i++)
   {
      if (const auto item = page().getPageItem(kDpadLabels[i]))
      {
         item->get().setVisible(i == pressed);
      }
   }

   if (const auto label = page().getPageItem<MenuPageLabelItem>(kDpadLabels[pressed]))
   {
      label->get().setAlpha(255);
   }
}

void ControllerTestView::showStick(int16_t x, int16_t y)
{
   // like processJoystickAxis(): both layers move from where they rest
   const auto dx = static_cast<int32_t>(kStickRange * x / 32767.0f);
   const auto dy = static_cast<int32_t>(kStickRange * y / 32767.0f);

   const auto place = [&](const std::string& name, std::optional<Position>& rest)
   {
      const auto item = page().getPageItem(name);
      const auto current_layer = item ? item->get().getCurrentLayer() : std::nullopt;
      if (!current_layer)
      {
         return;
      }

      PSD::Layer& layer = *current_layer;
      if (!rest)
      {
         rest = Position{layer.getLeft(), layer.getTop()};
      }

      layer.setX(rest->x + dx);
      layer.setY(rest->y + dy);
   };

   place(kStickActive, _stick_active_position);
   place(kStickOutline, _stick_outline_position);
}

void ControllerTestView::calibrate(const ControllerInput::State& state)
{
   auto& controls = GameSettings::getInstance().getControllerSettings();

   StickCalibration::Axes axes{};
   std::copy_n(state.axes.begin(), StickCalibration::axis_count, axes.begin());

   // like axesCalibrated(): steers right away, OK stores it, Cancel reloads the stored axes
   if (const auto pair = _calibration.process(axes, controls.getAnalogueThreshold()))
   {
      controls.setAnalogueAxis1(pair->first);
      controls.setAnalogueAxis2(pair->second);
      qDebug("ControllerTestView: steering axes %d, %d", pair->first, pair->second);
   }
}
