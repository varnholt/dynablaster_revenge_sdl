// dynablaster_revenge_harness - the test/diagnostic entry point, NOT the shipped game. Builds
// exactly the same engine/menu/effects code as the real game (src/main.cpp), but wraps it with
// CLI flags for headless screenshot verification (--selftest), scripted/synthetic input
// (--click, --realclick, --controller), page inspection (--dumplayer, --page, --dumppsd for a standalone PSD
// asset not part of the menu system), an isolated-effect view (--logo3d), the effect lab
// (--effect) and a standalone castle-level demo (the default mode with no flags at all). See
// CMakeLists.txt for how this and dynablaster_revenge share the same dynablaster_core library.
#include "gles3.h"
#include "glescontext.h"
#include "inputinjector.h"
#include "screenshot.h"

#include "gldevice.h"

#include "tools/datapaths.h"

#include "demofactory.h"
#include "effectlab.h"
#include "sdlglobaltime.h"

#include "engine/nodes/scenegraph.h"

#include "effects/spherefragments/spherefragmentsdrawable.h"
#include "game/gamelogodrawable.h"
#include "game/bombermanclient.h"
#include "game/controllerinput.h"
#include "game/controlspage.h"
#include "game/localplayers.h"
#include "game/menucontrollerhandler.h"

#include "image/image.h"
#include "image/psd.h"
#include "menus/bitmapfont.h"
#include "menus/fontmap.h"
#include "menus/fontpool.h"
#include "menus/gamefonts.h"
#include "menus/menu.h"
#include "menus/menudrawable.h"
#include "menus/menumousecursor.h"
#include "menus/menupage.h"
#include "menus/menupageeditablecomboboxitem.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <sstream>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace
{

/// \brief logs Menu::actionRequest so scripted button-click selftests (see --click/--dumplayer)
/// have observable proof that the menu-item click pipeline fired, not just "no crash".
struct ActionLogger
{
   void onActionRequest(const std::string& page, const std::string& action)
   {
      SDL_Log("Menu::actionRequest: page=%s action=%s", page.c_str(), action.c_str());
   }
};

/// \brief the scripted key sequence a headless --selftest run feeds into the game.
std::vector<InputInjector::Step> makeSelftestScript()
{
   return {
      {SDLK_RIGHT, 10},
      {SDLK_DOWN, 10},
      {SDLK_SPACE, 5},
   };
}

std::string argValue(const std::vector<std::string>& args, const std::string& prefix)
{
   const auto arg = std::ranges::find_if(args, [&prefix](const std::string& value) { return value.starts_with(prefix); });
   if (arg == args.end())
   {
      return {};
   }

   return arg->substr(prefix.size());
}

/// rief "up,down,left,right,bomb" -> controller buttons for a scripted --controller run
std::vector<SDL_GamepadButton> parseControllerScript(const std::string& text)
{
   std::vector<SDL_GamepadButton> buttons;
   std::stringstream stream(text);
   std::string step;
   while (std::getline(stream, step, ','))
   {
      if (step == "up")
      {
         buttons.push_back(SDL_GAMEPAD_BUTTON_DPAD_UP);
      }
      else if (step == "down")
      {
         buttons.push_back(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
      }
      else if (step == "left")
      {
         buttons.push_back(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
      }
      else if (step == "right")
      {
         buttons.push_back(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
      }
      else if (step == "bomb")
      {
         buttons.push_back(SDL_GAMEPAD_BUTTON_SOUTH);
      }
      else
      {
         SDL_Log("--controller: unknown step '%s'", step.c_str());
      }
   }
   return buttons;
}

bool hasFlag(const std::vector<std::string>& args, const std::string& flag)
{
   return std::ranges::find(args, flag) != args.end();
}

//! parses "x,y", leaves x and y untouched without a comma
void parseCoordinates(const std::string& text, int32_t& x, int32_t& y)
{
   const size_t comma = text.find(',');
   if (comma != std::string::npos)
   {
      x = std::atoi(text.substr(0, comma).c_str());
      y = std::atoi(text.substr(comma + 1).c_str());
   }
}

void dumpLayerPixels(const Image& image)
{
   const int32_t w = image.getWidth();
   const int32_t h = image.getHeight();
   const std::array<std::array<int32_t, 2>, 5> samples = {
      {{w / 2, h / 2}, {w / 4, h / 4}, {(3 * w) / 4, (3 * h) / 4}, {5, 5}, {w - 5, h - 5}}
   };
   for (const auto& sample : samples)
   {
      const uint32_t px = image.getScanline(sample[1])[sample[0]];
      SDL_Log(
         "  pixel(%d,%d): a=%d r=%d g=%d b=%d", sample[0], sample[1], (px >> 24) & 0xff, (px >> 16) & 0xff, (px >> 8) & 0xff, px & 0xff
      );
   }

   // scan row 0 for the first non-zero-alpha pixel and print its color.
   const uint32_t* row0 = image.getScanline(0);
   for (int32_t x = 0; x < w; x++)
   {
      const uint32_t px = row0[x];
      const uint32_t a = (px >> 24) & 0xff;
      if (a > 0)
      {
         SDL_Log("  first non-zero-alpha in row0 at x=%d: a=%d r=%d g=%d b=%d", x, a, (px >> 16) & 0xff, (px >> 8) & 0xff, px & 0xff);
         break;
      }
   }

   const auto has_alpha = [](uint32_t px) { return ((px >> 24) & 0xff) > 0; };

   // count how many pixels in row 0 have non-zero alpha, for density context.
   const auto non_zero = std::count_if(row0, row0 + w, has_alpha);
   SDL_Log("  row0 non-zero-alpha pixel count: %d / %d", static_cast<int32_t>(non_zero), w);

   // row 0 might just be a sparse top margin - check density + an example color at
   // several more representative rows across the image.
   for (const int32_t check_y : {100, 300, 540, 800, 1000})
   {
      const uint32_t* row = image.getScanline(check_y);
      const auto count = std::count_if(row, row + w, has_alpha);
      const uint32_t* example = std::find_if(row, row + w, has_alpha);
      const uint32_t example_pixel = (example != row + w) ? *example : 0;
      SDL_Log(
         "  row %d: non-zero count=%d/%d example a=%d r=%d g=%d b=%d",
         check_y,
         static_cast<int32_t>(count),
         w,
         (example_pixel >> 24) & 0xff,
         (example_pixel >> 16) & 0xff,
         (example_pixel >> 8) & 0xff,
         example_pixel & 0xff
      );
   }
}

}  // namespace

int main(int argc, char** argv)
{
   const std::vector<std::string> args(argv + 1, argv + argc);
   const bool selftest = hasFlag(args, "--selftest");
   const bool menu_mode = hasFlag(args, "--menu");
   // isolated view of the sphere-fragments "exploding earth/bomb" effect
   const bool logo3d_mode = hasFlag(args, "--logo3d");
   const std::string screenshot_path = argValue(args, "--screenshot=");
   const std::string click_arg = argValue(args, "--click=");
   const std::string dump_layer = argValue(args, "--dumplayer=");
   const std::string page_name = argValue(args, "--page=");
   const std::string dump_psd = argValue(args, "--dumppsd=");

   const std::string effect = argValue(args, "--effect=");
   if (!effect.empty())
   {
      const std::string out_dir = argValue(args, "--out=");
      const std::string level = argValue(args, "--level=");
      return runEffectLab(effect, out_dir.empty() ? "effect-lab" : out_dir, level.empty() ? "level-castle" : level);
   }

   if (!dump_psd.empty())
   {
      PSD psd;
      if (!psd.load(dump_psd))
      {
         SDL_Log("--dumppsd=%s: load failed", dump_psd.c_str());
         return 1;
      }
      SDL_Log("psd %s: %dx%d layers=%zu", dump_psd.c_str(), psd.getWidth(), psd.getHeight(), psd.getLayerCount());
      for (size_t i = 0; i < psd.getLayerCount(); i++)
      {
         const PSD::Layer& layer = psd.getLayer(i);
         SDL_Log(
            "  [%zu] name=%s group=%s left=%d top=%d w=%d h=%d opacity=%d visible=%d",
            i,
            layer.getName().c_str(),
            layer.getGroup().c_str(),
            layer.getLeft(),
            layer.getTop(),
            layer.getWidth(),
            layer.getHeight(),
            layer.getOpacity(),
            layer.isVisible()
         );
      }
      return 0;
   }

   GlesContext context;
   if (!context.init("dynablaster harness (SDL3 + GLES3)", 1280, 720))
   {
      return 1;
   }

   // shaders (loaded by GLDevice::loadShader) and textures (loaded by Image via loadtga) are
   // both plain file reads, resolved against these search paths rather than a hardcoded
   // prefix on every call site.
   DataPaths::add("data/shaders");
   DataPaths::add("data/textures");
   DataPaths::add("data/game");
   DataPaths::add("data/level-castle");
   DataPaths::add("data/logo");  // GameLogoPointSprite's "pointsprite" texture

   // constructing a RenderDevice sets the global activeDevice pointer (see renderdevice.cpp) -
   // every Material/VertexBuffer/etc call below goes through this.
   GLDevice device;
   device.init();
   device.resize(context.width(), context.height());
   device.setCulling(false);  // materials normally drive this per-draw via getCulling(); this demo doesn't wire that up

   SDL_Log("GL_VERSION: %s", glGetString(GL_VERSION));
   SDL_Log("GL_RENDERER: %s", glGetString(GL_RENDERER));
   SDL_Log("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

   InputInjector injector;
   if (selftest && !menu_mode && !logo3d_mode)
   {
      injector.queue(makeSelftestScript());
   }

   // animated materials and effects read this clock via GlobalTime::Instance()
   SdlGlobalTime global_time;

   // default mode: the castle level loaded through SceneGraph::load(), with the same material-ID
   // dispatch as LevelCastle::createMaterial. unused in --menu and --logo3d mode.
   SceneGraph scene;
   DemoMaterialFactory factory;

   // declared in reverse destruction order: cursor, logo, menu, logo3d
   std::unique_ptr<SphereFragmentsDrawable> logo3d;
   std::unique_ptr<MenuDrawable> menu_drawable;
   std::unique_ptr<GameLogoDrawable> logo_drawable;
   std::unique_ptr<MenuMouseCursor> menu_cursor;

   if (logo3d_mode)
   {
      logo3d = std::make_unique<SphereFragmentsDrawable>(&device);
      logo3d->initializeGL();
      logo3d->setVisible(true);
   }
   else if (menu_mode)
   {
      registerGameFonts();

      menu_drawable = std::make_unique<MenuDrawable>(&device);
      menu_drawable->initializeGL();
      menu_drawable->setVisible(true);

      menu_cursor = std::make_unique<MenuMouseCursor>(&device);
      menu_cursor->initializeGL();
      menu_cursor->setVisible(true);

      // the menu draws its own cursor (above, MenuMouseCursor) - hide the OS cursor so the two
      // don't overlap on screen.
      SDL_HideCursor();

      // the animated main-menu logo, only visible on the main menu page (see GameLogoDrawable::pageChanged())
      logo_drawable = std::make_unique<GameLogoDrawable>(&device);
      logo_drawable->initializeGL();
      logo_drawable->setVisible(true);
      menu_drawable->pageChangedSignal.connect([logo = logo_drawable.get()](const std::string& page) { logo->pageChanged(page); });

      // --page=<psd path>: jump straight to a page for a static verification screenshot. flips
      // isActive() directly, a static screenshot doesn't need the page transition.
      if (!page_name.empty())
      {
         MenuPage* target_page = menu_drawable->getMenu()->getPageByName(page_name.c_str());
         if (target_page)
         {
            MenuPage* previous = menu_drawable->getMenu()->getCurrentPage();
            if (previous && previous != target_page)
            {
               previous->setActive(false);
            }
            target_page->setActive(true);
            menu_drawable->getMenu()->setCurrentPage(target_page);
         }
         else
         {
            SDL_Log("--page=%s: no such page", page_name.c_str());
         }
      }

      // --dumplayer=<name>: print a page_1 PSD layer's page-space bounds and exit, used to find
      // click coordinates for scripted --click tests without guessing (MenuPage is-a PSD, so its
      // own layers - not just page items - are queryable by name).
      if (!dump_layer.empty())
      {
         const PSD::Layer* layer = nullptr;
         for (const MenuPage* page : {menu_drawable->getMenu()->getCurrentPage(), menu_drawable->getMenu()->getBackground()})
         {
            if (const auto found = page->getLayer(dump_layer); !layer && found != page->getLayers().end())
            {
               layer = &*found;
            }
         }
         if (layer)
         {
            SDL_Log(
               "layer %s: left=%d top=%d width=%d height=%d center=(%d,%d) opacity=%d",
               dump_layer.c_str(),
               layer->getLeft(),
               layer->getTop(),
               layer->getWidth(),
               layer->getHeight(),
               layer->getLeft() + layer->getWidth() / 2,
               layer->getTop() + layer->getHeight() / 2,
               layer->getOpacity()
            );

            // decoded pixel alpha at several points, separates psd decode issues from blending issues
            dumpLayerPixels(layer->getImage());

            // the gl texture ids the render path uses, to rule out a texture id mixup
            MenuPageItem* background_item = menu_drawable->getMenu()->getBackground()->getPageItem("background_active");
            if (background_item)
            {
               SDL_Log("  background page item's active-layer texture id=%u", background_item->getActiveLayer()->getTexture());
            }
            MenuPageItem* login_item = menu_drawable->getMenu()->getCurrentPage()->getPageItem("login_window");
            if (login_item)
            {
               SDL_Log("  login_window page item's active-layer texture id=%u", login_item->getActiveLayer()->getTexture());
            }
         }
         else
         {
            SDL_Log("layer %s: not found", dump_layer.c_str());
         }
         return 0;
      }

      static ActionLogger action_logger;
      menu_drawable->getMenu()->actionRequestSignal.connect([](const std::string& page, const std::string& action)
                                                            { action_logger.onActionRequest(page, action); });

      // placeholder rows so the host-address dropdown has something to render when opened
      MenuPageItem* host_table_item = menu_drawable->getMenu()->getCurrentPage()->getPageItem("editablecombobox_host_table");
      if (auto* host_table = dynamic_cast<MenuPageEditableComboBoxItem*>(host_table_item))
      {
         host_table->appendItem("127.0.0.1");
         host_table->appendItem("192.168.1.1");
         host_table->appendItem("10.0.0.5");
      }
   }
   else
   {
      const int32_t loaded = scene.load("level.hjb", &factory, nullptr);
      SDL_Log("scene.load(\"level.hjb\") -> %d, materials=%d", loaded, scene.getMaterialCount());
   }

   int32_t click_x = -1;
   int32_t click_y = -1;
   parseCoordinates(click_arg, click_x, click_y);

   // --realclick=x,y (WINDOW-space pixels, unlike --click which is page-space and calls the menu
   // handlers directly) - pushes genuine SDL_Event structs via SDL_PushEvent so they flow
   // through the same SDL_PollEvent loop and convertFromViewPort() conversion a real OS mouse
   // click would.
   // --controller=right,right,bomb (menu mode): drives the menu with a virtual controller through the
   // real ControllerInput/MenuControllerHandler path. a step's button is held for one frame, the
   // next step follows controller_step_ms later - the cursor glides in real time (up to 400 ms).
   constexpr uint64_t controller_step_ms = 600;
   size_t controller_step = 0;
   bool controller_held = false;
   uint64_t controller_step_start_ms = 0;
   const auto controller_script = parseControllerScript(argValue(args, "--controller="));
   std::unique_ptr<ControllerInput> controller_input;
   std::unique_ptr<MenuControllerHandler> menu_controller_handler;
   SDL_Joystick* virtual_controller = nullptr;

   // --controls [--pads=n] (menu mode): opens the controls page as if joining a game, with n
   // virtual controllers (default 1); --controller= steps are pressed on the first one
   const bool controls_mode = hasFlag(args, "--controls");
   const std::string pads_arg = argValue(args, "--pads=");
   const int32_t pad_count = controls_mode ? std::max(0, pads_arg.empty() ? 1 : std::atoi(pads_arg.c_str())) : 1;
   std::unique_ptr<BombermanClient> bomberman_client;
   std::unique_ptr<LocalPlayers> local_players;
   std::unique_ptr<ControlsPage> controls_page;
   bool controls_opened = false;

   if (menu_mode && menu_drawable && (!controller_script.empty() || controls_mode))
   {
      // the harness window usually isn't focused, SDL drops controller input then
      SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
      controller_input = std::make_unique<ControllerInput>();
      if (controller_input->initialize())
      {
         SDL_VirtualJoystickDesc desc;
         SDL_INIT_INTERFACE(&desc);
         desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
         desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
         desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
         desc.name = "harness controller";
         if (pad_count > 0)
         {
            virtual_controller = SDL_OpenJoystick(SDL_AttachVirtualJoystick(&desc));
         }
         for (int32_t pad = 1; pad < pad_count; pad++)
         {
            SDL_OpenJoystick(SDL_AttachVirtualJoystick(&desc));
         }
      }

      if (controls_mode)
      {
         bomberman_client = std::make_unique<BombermanClient>();
         local_players = std::make_unique<LocalPlayers>(*controller_input, *bomberman_client);
         controls_page = std::make_unique<ControlsPage>(*menu_drawable, *controller_input, *local_players, *bomberman_client);
         menu_drawable->getMenu()->actionRequestSignal.connect([page = controls_page.get()](const std::string& page_name, const std::string& action)
                                                               { page->onActionRequest(page_name, action); });
         controller_input->buttonPressedSignal.connect([page = controls_page.get()](ControllerInput::Id id, ControllerInput::Button button)
                                                       { page->onControllerButtonPressed(id, button); });
      }

      menu_controller_handler = std::make_unique<MenuControllerHandler>(*menu_drawable, *menu_cursor, *controller_input);
      menu_controller_handler->initialize();
      controller_input->buttonPressedSignal.connect(
         [handler = menu_controller_handler.get()](ControllerInput::Id, ControllerInput::Button button)
         {
            SDL_Log("controller: button 0x%x pressed", button);
            handler->buttonPressed(button);
         }
      );
      menu_drawable->pageChangedSignal.connect([handler = menu_controller_handler.get()](const std::string&)
                                               { handler->focusDefaultItem(); });
   }

   const std::string realclick_arg = argValue(args, "--realclick=");
   int32_t real_click_x = -1;
   int32_t real_click_y = -1;
   parseCoordinates(realclick_arg, real_click_x, real_click_y);

   bool running = true;
   int32_t frame = 0;

   // --selftest renders frames as fast as the host can, swap interval doesn't throttle a
   // headless context. MenuDrawable's fade-in is driven by wall-clock dt, so feed it a fixed
   // synthetic 60fps timestep to keep the selftest deterministic.
   float menu_time_ms = 0.0f;

   while (running)
   {
      SDL_Event event{};
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_EVENT_QUIT)
         {
            running = false;
         }

         if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
         {
            running = false;
         }

         if (controller_input)
         {
            controller_input->handleEvent(event);
         }

         if (menu_mode && menu_drawable)
         {
            // menu items live in 1920x1080 page space, not window space
            switch (event.type)
            {
               case SDL_EVENT_MOUSE_MOTION:
               {
                  int x = static_cast<int>(event.motion.x);
                  int y = static_cast<int>(event.motion.y);
                  device.convertFromViewPort(&x, &y, 1920, 1080);
                  menu_drawable->mouseMoveEvent(x, y);
                  if (menu_cursor)
                  {
                     menu_cursor->mouseMoveEvent(x, y);
                  }
                  break;
               }
               case SDL_EVENT_MOUSE_BUTTON_DOWN:
               {
                  int x = static_cast<int>(event.button.x);
                  int y = static_cast<int>(event.button.y);
                  device.convertFromViewPort(&x, &y, 1920, 1080);
                  menu_drawable->mousePressEvent(x, y);
                  if (menu_cursor)
                  {
                     menu_cursor->mousePressEvent(x, y);
                  }
                  break;
               }
               case SDL_EVENT_MOUSE_BUTTON_UP:
                  menu_drawable->mouseReleaseEvent();
                  if (menu_cursor)
                  {
                     menu_cursor->mouseReleaseEvent();
                  }
                  break;
               default:
                  break;
            }
         }
      }

      if (selftest && !menu_mode && !logo3d_mode)
      {
         injector.advance();
      }

      // scripted click for --menu --selftest --click=x,y verification runs: move there first
      // (so hover/focus state updates the same way a real mouse would), then press+release a
      // couple frames later so the item's activated()/mousePressed() path is exercised for real
      // rather than skipped.
      if (menu_mode && menu_drawable && click_x >= 0)
      {
         if (frame == 15)
         {
            menu_drawable->mouseMoveEvent(click_x, click_y);
            if (menu_cursor)
            {
               menu_cursor->mouseMoveEvent(click_x, click_y);
            }
         }
         else if (frame == 20)
         {
            menu_drawable->mousePressEvent(click_x, click_y);
            if (menu_cursor)
            {
               menu_cursor->mousePressEvent(click_x, click_y);
            }
         }
         else if (frame == 22)
         {
            menu_drawable->mouseReleaseEvent();
            if (menu_cursor)
            {
               menu_cursor->mouseReleaseEvent();
            }
         }
      }

      // --controller: press, release on the next frame, then wait for the next step
      if (virtual_controller && !controller_input->getDevices().empty() && controller_step < controller_script.size())
      {
         const uint64_t now_ms = SDL_GetTicks();
         if (controller_held)
         {
            SDL_SetJoystickVirtualButton(virtual_controller, controller_script[controller_step], false);
            controller_held = false;
            ++controller_step;
         }
         else if (now_ms - controller_step_start_ms >= controller_step_ms)
         {
            SDL_SetJoystickVirtualButton(virtual_controller, controller_script[controller_step], true);
            controller_held = true;
            controller_step_start_ms = now_ms;
         }
      }

      // the controls page needs its controllers connected
      if (controls_page && !controls_opened && controller_input->getDevices().size() == static_cast<size_t>(pad_count))
      {
         controls_opened = controls_page->open(1, "data/menus/mainmenu.psd");
      }

      if (controller_input)
      {
         SDL_UpdateJoysticks();
         controller_input->poll();
         menu_controller_handler->update();
         if (controls_page)
         {
            controls_page->update();
         }
      }

      // --realclick=x,y: same frame schedule as --click above, but through genuine SDL events
      if (menu_mode && real_click_x >= 0)
      {
         const SDL_WindowID window_id = SDL_GetWindowID(context.window());

         if (frame == 15)
         {
            SDL_Event motion_event{};
            motion_event.type = SDL_EVENT_MOUSE_MOTION;
            motion_event.motion.windowID = window_id;
            motion_event.motion.x = static_cast<float>(real_click_x);
            motion_event.motion.y = static_cast<float>(real_click_y);
            SDL_PushEvent(&motion_event);
         }
         else if (frame == 20)
         {
            SDL_Event down_event{};
            down_event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            down_event.button.windowID = window_id;
            down_event.button.button = SDL_BUTTON_LEFT;
            down_event.button.down = true;
            down_event.button.clicks = 1;
            down_event.button.x = static_cast<float>(real_click_x);
            down_event.button.y = static_cast<float>(real_click_y);
            SDL_PushEvent(&down_event);
         }
         else if (frame == 22)
         {
            SDL_Event up_event{};
            up_event.type = SDL_EVENT_MOUSE_BUTTON_UP;
            up_event.button.windowID = window_id;
            up_event.button.button = SDL_BUTTON_LEFT;
            up_event.button.down = false;
            up_event.button.clicks = 1;
            up_event.button.x = static_cast<float>(real_click_x);
            up_event.button.y = static_cast<float>(real_click_y);
            SDL_PushEvent(&up_event);
         }
      }

      device.clear();

      if (logo3d_mode && logo3d)
      {
         global_time.update();
         logo3d->paintGL();
      }
      else if (menu_mode && menu_drawable)
      {
         if (selftest)
         {
            menu_time_ms += 16.6667f;
         }
         else
         {
            menu_time_ms = static_cast<float>(SDL_GetTicks());
         }

         menu_drawable->animate(menu_time_ms);
         menu_drawable->paintGL();

         if (menu_cursor && menu_cursor->isVisible())
         {
            menu_cursor->animate(menu_time_ms);
            menu_cursor->paintGL();
         }

         if (logo_drawable && logo_drawable->isVisible())
         {
            // the logo animates in real seconds * 62.5 (ms * 0.0625), its sphere rotation reads
            // GlobalTime directly
            global_time.update();
            logo_drawable->animate(menu_time_ms * 0.0625f);
            logo_drawable->paintGL();
         }
      }
      else
      {
         global_time.update();
         scene.render();
      }

      context.swap();

      ++frame;

      const bool controller_done = controller_script.empty() ||
                                   (controller_step == controller_script.size() && SDL_GetTicks() - controller_step_start_ms > controller_step_ms);
      const bool selftest_done = (menu_mode || logo3d_mode) ? (selftest && frame > 40 && controller_done)
                                                            : (selftest && injector.finished() && frame > 40);

      if (selftest_done && controls_page)
      {
         const auto& columns = controls_page->getSetup().getColumns();
         for (size_t i = 0; i < columns.size(); i++)
         {
            const auto& column = columns[i];
            const char* type = column.device.type == ControlsSetup::DeviceType::Keyboard     ? "keyboard"
                               : column.device.type == ControlsSetup::DeviceType::Controller ? "controller"
                                                                                             : "none";
            SDL_Log("controls: column %zu %s %u color %d", i + 1, type, column.device.id, static_cast<int>(column.color));
         }
      }

      if (selftest_done)
      {
         if (!screenshot_path.empty())
         {
            saveScreenshot(screenshot_path, context.width(), context.height());
         }

         running = false;
      }
   }

   return 0;
}
