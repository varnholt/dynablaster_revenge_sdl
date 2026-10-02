// drives ControllerInput with an SDL virtual controller: hotplug, buttons, stick directions and the
// release of held buttons on disconnect.
#include "game/controllerinput.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

namespace
{
struct Recorder
{
   std::vector<ControllerInput::Id> added;
   std::vector<ControllerInput::Id> removed;
   uint32_t held = 0;
   uint32_t released = 0;
};

bool check(bool condition, const char* what)
{
   if (!condition)
   {
      SDL_Log("FAILED: %s", what);
   }
   return condition;
}

void pump(ControllerInput& input)
{
   SDL_UpdateJoysticks();
   SDL_Event event{};
   while (SDL_PollEvent(&event))
   {
      input.handleEvent(event);
   }
   input.poll();
}
}  // namespace

int main(int /*argc*/, char** /*argv*/)
{
   ControllerInput input;
   if (!input.initialize())
   {
      SDL_Log("FAILED: controller subsystem unavailable");
      return 1;
   }

   Recorder recorder;
   input.deviceAddedSignal.connect([&](ControllerInput::Id id) { recorder.added.push_back(id); });
   input.deviceRemovedSignal.connect([&](ControllerInput::Id id) { recorder.removed.push_back(id); });
   input.buttonPressedSignal.connect([&](ControllerInput::Id, ControllerInput::Button button) { recorder.held |= button; });
   input.buttonReleasedSignal.connect(
      [&](ControllerInput::Id, ControllerInput::Button button)
      {
         recorder.held &= ~button;
         recorder.released |= button;
      }
   );

   SDL_VirtualJoystickDesc desc;
   SDL_INIT_INTERFACE(&desc);
   desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
   desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
   desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
   desc.name = "virtual test controller";
   const ControllerInput::Id id = SDL_AttachVirtualJoystick(&desc);
   SDL_Joystick* controller = SDL_OpenJoystick(id);

   bool ok = check(id != 0 && controller, "attach virtual controller");
   pump(input);
   ok &= check(recorder.added.size() == 1 && recorder.added.front() == id, "hotplug add");
   ok &= check(input.getDevices().size() == 1, "device listed");

   SDL_SetJoystickVirtualButton(controller, SDL_GAMEPAD_BUTTON_SOUTH, true);
   pump(input);
   ok &= check((recorder.held & ControllerInput::ButtonBomb) != 0, "south pressed");

   SDL_SetJoystickVirtualButton(controller, SDL_GAMEPAD_BUTTON_SOUTH, false);
   pump(input);
   ok &= check((recorder.released & ControllerInput::ButtonBomb) != 0, "south released");

   SDL_SetJoystickVirtualAxis(controller, SDL_GAMEPAD_AXIS_LEFTX, -32767);
   pump(input);
   ok &= check(input.getButtons(id) == ControllerInput::ButtonLeft, "stick left is a direction");

   SDL_SetJoystickVirtualAxis(controller, SDL_GAMEPAD_AXIS_LEFTX, 100);
   pump(input);
   ok &= check(input.getButtons(id) == 0, "stick inside the dead zone");

   SDL_SetJoystickVirtualButton(controller, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
   pump(input);
   recorder.released = 0;
   SDL_CloseJoystick(controller);
   SDL_DetachVirtualJoystick(id);
   pump(input);
   ok &= check(recorder.removed.size() == 1, "hotplug remove");
   ok &= check((recorder.released & ControllerInput::ButtonUp) != 0, "held button released on disconnect");
   ok &= check(input.getDevices().empty(), "device gone");

   SDL_Log(ok ? "controller input test passed" : "controller input test failed");
   return ok ? 0 : 1;
}
