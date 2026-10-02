// the controls page's model: one column per device, unique devices and colors, and a setup
// remembered per controller count that finds its controllers again by guid.
#include "game/controlssetup.h"
#include "settings.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <vector>

namespace
{
using Type = ControlsSetup::DeviceType;

bool check(bool condition, const char* what)
{
   if (!condition)
   {
      SDL_Log("FAILED: %s", what);
   }
   return condition;
}

ControllerInput::DeviceInfo pad(ControllerInput::Id id, const char* guid)
{
   return {id, "pad", guid};
}
}  // namespace

int main(int /*argc*/, char** /*argv*/)
{
   bool ok = true;
   const std::vector<std::string> names = {"one", "two", "three", "four", "five"};

   ControlsSetup setup;
   setup.reset({pad(7, "aaaa"), pad(9, "bbbb")}, 5, names);
   const auto& columns = setup.getColumns();

   ok &= check(columns.size() == 3, "keyboard plus one column per controller");
   ok &= check(columns[0].device.type == Type::Keyboard && columns[1].device.id == 7 && columns[2].device.id == 9, "devices in order");
   ok &= check(columns[0].color == Constants::ColorWhite && columns[1].color == Constants::ColorBlack, "colors in order");
   ok &= check(columns[2].name == "three", "names in order");
   ok &= check(setup.getPlayingColumns().size() == 3, "all play");

   // colors stay unique
   setup.cycleColor(0, 1);
   ok &= check(columns[0].color == Constants::ColorGreen, "color skips the taken ones");
   setup.cycleColor(0, -1);
   ok &= check(columns[0].color == Constants::ColorWhite, "and goes back");
   setup.cycleColor(1, -1);
   ok &= check(columns[1].color == Constants::ColorOrange, "colors wrap around");

   // a controller becomes player one
   setup.moveDevice(1, -1);
   ok &= check(columns[0].device.id == 7 && columns[1].device.type == Type::Keyboard, "move swaps with the neighbour");
   setup.moveDevice(0, -1);
   ok &= check(columns[0].device.id == 7, "nothing left of the first column");

   // cycling devices swaps with the column that has it
   setup.cycleDevice(2, -1);  // controller 9 -> controller 7
   ok &= check(columns[2].device.id == 7 && columns[0].device.id == 9, "cycle swaps");
   setup.assign(1, {});
   ok &= check(columns[1].device.type == Type::None && setup.getPlayingColumns().size() == 2, "a column can sit out");
   ok &= check(!setup.findColumn(ControlsSetup::keyboard()), "the keyboard is in no column");

   // remembered per controller count, controllers found by guid even if their ids changed
   const char* file = "test_controls_setup.ini";
   std::remove(file);
   {
      Settings settings(file);
      setup.setName(0, "renamed");
      setup.store(settings, 2);
   }
   {
      Settings settings(file);
      ControlsSetup restored;
      ok &= check(!restored.restore(settings, {pad(1, "aaaa")}, 5), "nothing stored for one controller");
      ok &= check(restored.restore(settings, {pad(3, "bbbb"), pad(4, "aaaa")}, 5), "stored for two controllers");
      const auto& restored_columns = restored.getColumns();
      ok &= check(restored_columns.size() == 3, "column count restored");
      ok &= check(restored_columns[0].device.id == 3 && restored_columns[2].device.id == 4, "controllers found by guid");
      ok &= check(restored_columns[1].device.type == Type::None, "the empty column stays empty");
      ok &= check(restored_columns[0].name == "renamed", "names restored");
      ok &= check(restored_columns[0].color == columns[0].color && restored_columns[2].color == columns[2].color, "colors restored");

      ControlsSetup swapped;
      swapped.restore(settings, {pad(5, "aaaa"), pad(6, "cccc")}, 5);
      ok &= check(swapped.getColumns()[2].device.id == 5, "a known controller keeps its column");
      ok &= check(swapped.findColumn({Type::Controller, 6, "cccc"}).has_value(), "an unknown controller fills a free column");
   }
   std::remove(file);

   SDL_Log(ok ? "controls setup test passed" : "controls setup test failed");
   return ok ? 0 : 1;
}
