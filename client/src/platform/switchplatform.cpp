#include "switchplatform.h"

#include "framework/keyevent.h"
#include "game/gamedrawable.h"
#include "game/gamesettings.h"
#include "menus/menu.h"
#include "menus/menudrawable.h"
#include "menus/menumousecursor.h"
#include "menus/menupage.h"
#include "menus/menupageeditablecomboboxitem.h"
#include "menus/menupagetextedit.h"

#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

namespace
{
const std::string settings_path = "sdmc:/switch/dynablaster_revenge/game.ini";

struct FileCloser
{
   void operator()(FILE* file) const
   {
      std::fclose(file);
   }
};

using FileHandle = std::unique_ptr<FILE, FileCloser>;

bool makeDirectory(const std::string& path)
{
   return mkdir(path.c_str(), 0777) == 0 || errno == EEXIST;
}

bool seedSettings()
{
   if (FileHandle existing{std::fopen(settings_path.c_str(), "rb")})
   {
      return true;
   }
   FileHandle source{std::fopen("data/game.ini", "rb")};
   if (!source)
      return false;
   FileHandle destination{std::fopen(settings_path.c_str(), "wb")};
   if (!destination)
   {
      return false;
   }
   std::array<char, 4096> buffer;
   bool ok = true;
   while (const size_t count = std::fread(buffer.data(), 1, buffer.size(), source.get()))
   {
      if (std::fwrite(buffer.data(), 1, count, destination.get()) != count)
      {
         ok = false;
         break;
      }
   }
   ok = !std::ferror(source.get()) && ok;
   source.reset();
   ok = std::fclose(destination.release()) == 0 && ok;
   if (!ok)
      std::remove(settings_path.c_str());
   return ok;
}
}  // namespace

bool SwitchPlatform::initialize()
{
   _romfs = R_SUCCEEDED(romfsInit());
   if (!_romfs || chdir("romfs:/") != 0)
      return false;
   if (!makeDirectory("sdmc:/switch") || !makeDirectory("sdmc:/switch/dynablaster_revenge") || !seedSettings())
      return false;

   // Preserve startup/shader/network diagnostics even without an nxlink connection.
   std::freopen("sdmc:/switch/dynablaster_revenge/game.log", "w", stderr);
   std::setvbuf(stderr, nullptr, _IONBF, 0);
   std::freopen("sdmc:/switch/dynablaster_revenge/application.log", "w", stdout);
   std::setvbuf(stdout, nullptr, _IONBF, 0);
   _sockets = R_SUCCEEDED(socketInitializeDefault());
   if (!_sockets)
   {
      SDL_Log("Switch socket service failed to initialize");
      return false;
   }
   _nifm = R_SUCCEEDED(nifmInitialize(NifmServiceType_User));
   padConfigureInput(1, HidNpadStyleSet_NpadStandard);
   padInitializeDefault(&_pad);
   return true;
}

SwitchPlatform::~SwitchPlatform()
{
   if (_nifm)
      nifmExit();
   if (_sockets)
      socketExit();
   if (_romfs)
      romfsExit();
}

void SwitchPlatform::editText(MenuDrawable& menu)
{
   const auto page = menu.getMenu().getCurrentPage();
   if (!page)
      return;
   const auto item = page->get().getActiveItem();
   if (!item)
      return;
   std::optional<std::reference_wrapper<MenuPageTextEditItem>> text_edit;
   if (auto* text_edit_item = dynamic_cast<MenuPageTextEditItem*>(&item->get()))
      text_edit = *text_edit_item;
   if (auto* combo = dynamic_cast<MenuPageEditableComboBoxItem*>(&item->get()))
      text_edit = combo->getTextEditItem();
   if (!text_edit)
      return;
   MenuPageTextEditItem& edit = *text_edit;

   SwkbdConfig keyboard{};
   if (R_FAILED(swkbdCreate(&keyboard, 0)))
      return;
   swkbdConfigMakePresetDefault(&keyboard);
   swkbdConfigSetInitialText(&keyboard, edit.getText().c_str());
   swkbdConfigSetStringLenMax(&keyboard, std::clamp(edit.getMaxLength(), 1, 255));
   std::array<char, 1024> text{};
   const Result result = swkbdShow(&keyboard, text.data(), text.size());
   swkbdClose(&keyboard);
   if (R_SUCCEEDED(result))
      edit.setText(text.data());
   _last_tick = SDL_GetTicks();
}

void SwitchPlatform::updateInput(bool in_game, GameDrawable& game, MenuDrawable& menu, MenuMouseCursor& cursor)
{
   padUpdate(&_pad);
   const u64 held = padGetButtons(&_pad);
   const u64 down = padGetButtonsDown(&_pad);
   const u64 up = padGetButtonsUp(&_pad);
   const HidAnalogStickState stick = padGetStickPos(&_pad, 0);
   constexpr int dead_zone = 6500;
   const bool left = (held & HidNpadButton_Left) || stick.x < -dead_zone;
   const bool right = (held & HidNpadButton_Right) || stick.x > dead_zone;
   const bool above = (held & HidNpadButton_Up) || stick.y > dead_zone;
   const bool below = (held & HidNpadButton_Down) || stick.y < -dead_zone;

   // Releases on mode changes and disconnects prevent a stuck movement/bomb key.
   std::array<SDL_Keycode, 10> keys{};
   if (in_game)
   {
      auto& controls = GameSettings::getInstance().getControllerSettings();
      keys = {
         above ? controls.getUpKey() : 0,
         below ? controls.getDownKey() : 0,
         left ? controls.getLeftKey() : 0,
         right ? controls.getRightKey() : 0,
         (held & (HidNpadButton_A | HidNpadButton_B)) ? controls.getBombKey() : 0,
         (held & HidNpadButton_L) ? controls.getZoomOutKey() : 0,
         (held & HidNpadButton_R) ? controls.getZoomInKey() : 0,
         (held & HidNpadButton_ZL) ? SDLK_TAB : 0,
         (held & HidNpadButton_Minus) ? SDLK_ESCAPE : 0,
         (held & HidNpadButton_Plus) ? SDLK_F10 : 0
      };
   }
   for (size_t i = 0; i < keys.size(); ++i)
   {
      if (_held_keys[i] != keys[i])
      {
         if (_held_keys[i])
            game.keyReleaseEvent(KeyEvent(_held_keys[i], {}, false));
         if (keys[i])
            game.keyPressEvent(KeyEvent(keys[i], {}, false));
      }
   }
   _held_keys = keys;
   if (in_game != _in_game)
   {
      menu.mouseReleaseEvent();
      cursor.mouseReleaseEvent();
      _in_game = in_game;
   }

   const uint64_t now = SDL_GetTicks();
   const float dt = std::min(static_cast<float>(now - _last_tick) * 0.001f, 0.05f);
   _last_tick = now;
   if (in_game)
      return;

   const float dx = (held & HidNpadButton_Left)     ? -1.0f
                    : (held & HidNpadButton_Right)  ? 1.0f
                    : std::abs(stick.x) > dead_zone ? static_cast<float>(stick.x) / 32768.0f
                                                    : 0.0f;
   const float dy = (held & HidNpadButton_Up)       ? -1.0f
                    : (held & HidNpadButton_Down)   ? 1.0f
                    : std::abs(stick.y) > dead_zone ? -static_cast<float>(stick.y) / 32768.0f
                                                    : 0.0f;
   _cursor_x = std::clamp(_cursor_x + dx * dt * 1200.0f, 0.0f, 1919.0f);
   _cursor_y = std::clamp(_cursor_y + dy * dt * 1200.0f, 0.0f, 1079.0f);
   const int x = static_cast<int>(_cursor_x);
   const int y = static_cast<int>(_cursor_y);
   menu.mouseMoveEvent(x, y);
   cursor.mouseMoveEvent(x, y);
   if (down & HidNpadButton_A)
   {
      menu.mousePressEvent(x, y);
      cursor.mousePressEvent(x, y);
   }
   if (up & HidNpadButton_A)
   {
      menu.mouseReleaseEvent();
      cursor.mouseReleaseEvent();
   }
   if (down & HidNpadButton_X)
      editText(menu);
   if (down & HidNpadButton_B)
   {
      if (const auto page = menu.getMenu().getCurrentPage())
      {
         for (const auto& item : page->get().getPageItems())
         {
            const auto current_layer = item->getCurrentLayer();
            if (!current_layer || !item->isVisible() || !item->isEnabled())
               continue;
            const PSD::Layer& layer = *current_layer;
            const std::string name = layer.getName();
            if (name.starts_with("button_back") || name.starts_with("button_cancel") || name.starts_with("button_leave"))
            {
               const int bx = layer.getLeft() + layer.getWidth() / 2;
               const int by = layer.getTop() + layer.getHeight() / 2;
               menu.mouseMoveEvent(bx, by);
               menu.mousePressEvent(bx, by);
               menu.mouseReleaseEvent();
               break;
            }
         }
      }
   }
}
