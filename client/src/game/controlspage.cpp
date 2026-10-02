#include "controlspage.h"

#include "bombermanclient.h"
#include "gamesettings.h"
#include "settings.h"

#include "menus/menu.h"
#include "menus/menudrawable.h"
#include "menus/menupage.h"
#include "menus/menupageitem.h"
#include "menus/menupagelabelitem.h"
#include "menus/menupagetextedit.h"

#include <array>

namespace
{
constexpr const char* controls_page = "data/menus/controls.psd";
constexpr const char* column_group = "column";
constexpr int32_t page_width = 1920;
constexpr int32_t column_width = 193;
constexpr int32_t column_spacing = 240;

constexpr const char* arrow_color_left = "Shape 14 copy 6";
constexpr const char* arrow_color_right = "Shape 14 copy 7";
constexpr const char* arrow_device_left = "Shape 14 copy 4";
constexpr const char* arrow_device_right = "Shape 14 copy 5";
constexpr const char* action_ok = "button_ok_active";
constexpr const char* action_cancel = "button_cancel_active";

// dstar's helmets follow Constants::Color, except green and blue which are swapped in the PSD
std::string colorIcon(Constants::Color color)
{
   int32_t index = static_cast<int32_t>(color);
   if (color == Constants::ColorGreen)
   {
      index = Constants::ColorBlue;
   }
   else if (color == Constants::ColorBlue)
   {
      index = Constants::ColorGreen;
   }
   return "p" + std::to_string(index) + "-icon";
}

// "Shape 14 copy 6@3" -> {"Shape 14 copy 6", 2}
std::pair<std::string, size_t> splitInstance(const std::string& name)
{
   const std::string base = MenuPage::getInstanceBaseName(name);
   if (base == name)
   {
      return {name, 0};
   }
   return {base, static_cast<size_t>(std::stoi(name.substr(base.size() + 1)) - 1)};
}
}  // namespace

ControlsPage::ControlsPage(MenuDrawable& menu, ControllerInput& controller_input, LocalPlayers& local_players, BombermanClient& client)
    : _menu(menu), _controller_input(controller_input), _local_players(local_players), _client(client)
{
   if (MenuPage* page = getPage())
   {
      const auto first_column = page->getLayer(MenuPage::getInstanceName("controls_window", 1));
      _column_left = first_column != page->getLayers().end() ? first_column->getLeft() : 0;
   }
}

ControlsPage::~ControlsPage()
{
   if (_join_connected)
   {
      _client.joinGameResponseSignal.disconnect(_join_connection);
   }
}

MenuPage* ControlsPage::getPage() const
{
   return Menu::getInstance()->getPageByName(controls_page);
}

const ControlsSetup& ControlsPage::getSetup() const
{
   return _setup;
}

bool ControlsPage::isCurrentPage() const
{
   const MenuPage* current = Menu::getInstance()->getCurrentPage();
   return current && current->getFilename() == controls_page;
}

std::vector<std::string> ControlsPage::getDefaultNames() const
{
   const auto* login = GameSettings::getInstance()->getLoginSettings();
   return {
      login->getNick(),
      login->getPlayer2Nick(),
      login->getPlayer3Nick(),
      login->getPlayer4Nick(),
      login->getPlayer5Nick(),
      login->getPlayer6Nick(),
      login->getPlayer7Nick(),
      login->getPlayer8Nick(),
      login->getPlayer9Nick(),
      login->getPlayer10Nick(),
   };
}

bool ControlsPage::open(int32_t game_id, const std::string& return_page)
{
   MenuPage* page = getPage();
   const auto controllers = _controller_input.getDevices();
   if (!page)
   {
      return false;
   }

   _game_id = game_id;
   _return_page = return_page;

   const auto max_columns = static_cast<size_t>(page->getGroupInstanceCount(column_group));
   Settings settings(GameSettings::getFilename());
   // a stored setup where nobody plays is of no use
   if (!_setup.restore(settings, controllers, max_columns) || _setup.getPlayingColumns().empty())
   {
      _setup.reset(controllers, max_columns, getDefaultNames());
   }

   // the main player starts with the nick it logged in with
   if (const auto playing = _setup.getPlayingColumns(); !playing.empty())
   {
      _setup.setName(playing.front(), GameSettings::getInstance()->getLoginSettings()->getNick());
   }

   // the names belong to the columns, not to whoever edited them last
   for (size_t i = 0; i < _setup.getColumns().size(); i++)
   {
      if (MenuPageTextEditItem* edit =
             dynamic_cast<MenuPageTextEditItem*>(page->getPageItem(MenuPage::getInstanceName("bg_linedit_name", static_cast<int32_t>(i + 1))
             )))
      {
         edit->setText(_setup.getColumns()[i].name);
      }
   }

   refresh();
   _menu.pageChangeRequest(controls_page);
   return true;
}

void ControlsPage::readNames()
{
   MenuPage* page = getPage();
   for (size_t i = 0; i < _setup.getColumns().size(); i++)
   {
      if (const auto* edit =
             dynamic_cast<MenuPageTextEditItem*>(page->getPageItem(MenuPage::getInstanceName("bg_linedit_name", static_cast<int32_t>(i + 1))
             )))
      {
         _setup.setName(i, edit->getText());
      }
   }
}

void ControlsPage::refresh()
{
   MenuPage* page = getPage();
   if (!page)
   {
      return;
   }

   const auto& columns = _setup.getColumns();
   const auto instances = page->getGroupInstanceCount(column_group);
   const auto count = static_cast<int32_t>(columns.size());

   // as many columns as players, centered
   const int32_t left = (page_width - ((count - 1) * column_spacing + column_width)) / 2;

   const auto playing = _setup.getPlayingColumns();

   for (int32_t index = 1; index <= instances; index++)
   {
      page->setGroupInstanceOffset(column_group, index, left - _column_left);

      const bool shown = index <= count;
      const ControlsSetup::Column* column = shown ? &columns[static_cast<size_t>(index - 1)] : nullptr;
      const bool plays = column && column->device.type != ControlsSetup::DeviceType::None;

      const auto set_visible = [&](const std::string& base, bool visible)
      {
         if (MenuPageItem* item = page->getPageItem(MenuPage::getInstanceName(base, index)))
         {
            item->setVisible(visible);
         }
      };

      set_visible("controls_window", shown);
      for (const char* base : {arrow_device_left, arrow_device_right})
      {
         set_visible(base, shown && _setup.canCycleDevice(static_cast<size_t>(index - 1)));
      }
      for (const char* base : {"player-select", "bg_linedit_name", "player_number", arrow_color_left, arrow_color_right})
      {
         set_visible(base, plays);
      }
      set_visible("keyboard-icon", column && column->device.type == ControlsSetup::DeviceType::Keyboard);
      set_visible("gamepad-icon", column && column->device.type == ControlsSetup::DeviceType::Controller);
      for (int32_t level = 0; level < 4; level++)
      {
         set_visible("bot-level" + std::to_string(level), false);
      }
      for (int32_t color = 1; color <= ControlsSetup::color_count; color++)
      {
         set_visible(colorIcon(static_cast<Constants::Color>(color)), plays && column->color == static_cast<Constants::Color>(color));
      }

      // the player number in joining order
      if (auto* label = dynamic_cast<MenuPageLabelItem*>(page->getPageItem(MenuPage::getInstanceName("player_number", index))))
      {
         const auto position = std::ranges::find(playing, static_cast<size_t>(index - 1));
         label->setText(position != playing.end() ? std::to_string(position - playing.begin() + 1) : std::string());
      }
   }
}

void ControlsPage::identify(size_t column)
{
   // the controller now in this column rumbles, so its player knows which one it is
   const auto& device = _setup.getColumns()[column].device;
   if (device.type == ControlsSetup::DeviceType::Controller)
   {
      _controller_input.rumble(device.id, 0.4f, 250);
   }
}

void ControlsPage::onActionRequest(const std::string& page, const std::string& action)
{
   if (page != controls_page)
   {
      return;
   }

   if (action == action_ok)
   {
      confirm();
      return;
   }
   if (action == action_cancel)
   {
      cancel();
      return;
   }

   const auto [base, column] = splitInstance(action);
   if (column >= _setup.getColumns().size())
   {
      return;
   }

   readNames();
   if (base == arrow_color_left || base == arrow_color_right)
   {
      _setup.cycleColor(column, base == arrow_color_left ? -1 : 1);
   }
   else if (base == arrow_device_left || base == arrow_device_right)
   {
      _setup.cycleDevice(column, base == arrow_device_left ? -1 : 1);
      identify(column);
   }
   refresh();
}

void ControlsPage::onControllerButtonPressed(ControllerInput::Id id, ControllerInput::Button button)
{
   if (!isCurrentPage())
   {
      return;
   }

   const auto devices = _controller_input.getDevices();
   const auto info = std::ranges::find(devices, id, &ControllerInput::DeviceInfo::id);
   if (info == devices.end())
   {
      return;
   }

   readNames();
   const auto device = ControlsSetup::controller(*info);
   auto column = _setup.findColumn(device);

   // a controller without a column takes the first free one
   if (!column)
   {
      const auto& columns = _setup.getColumns();
      const auto free = std::ranges::find_if(columns, [](const auto& c) { return c.device.type == ControlsSetup::DeviceType::None; });
      if (free == columns.end())
      {
         return;
      }
      const auto free_column = static_cast<size_t>(free - columns.begin());
      _setup.assign(free_column, device);
      identify(free_column);
      refresh();
      return;
   }

   switch (button)
   {
      case ControllerInput::ButtonLeft:
      case ControllerInput::ButtonRight:
         _setup.cycleColor(*column, button == ControllerInput::ButtonLeft ? -1 : 1);
         break;
      case ControllerInput::ButtonUp:
      case ControllerInput::ButtonDown:
         _setup.moveDevice(*column, button == ControllerInput::ButtonUp ? -1 : 1);
         identify(*_setup.findColumn(device));
         break;
      case ControllerInput::ButtonBomb:
      case ControllerInput::ButtonStart:
         confirm();
         return;
      default:
         break;
   }
   refresh();
}

bool ControlsPage::onKeyPressed(SDL_Keycode key)
{
   if (!isCurrentPage())
   {
      return false;
   }

   // typing a name
   if (const auto* edit = dynamic_cast<MenuPageTextEditItem*>(getPage()->getActiveItem()); edit && edit->isEditingActive())
   {
      return false;
   }

   readNames();
   const auto column = _setup.findColumn(ControlsSetup::keyboard());
   switch (key)
   {
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
         confirm();
         return true;
      case SDLK_ESCAPE:
         cancel();
         return true;
      case SDLK_LEFT:
      case SDLK_RIGHT:
         if (column)
         {
            _setup.cycleColor(*column, key == SDLK_LEFT ? -1 : 1);
            refresh();
         }
         return true;
      case SDLK_UP:
      case SDLK_DOWN:
         if (column)
         {
            _setup.moveDevice(*column, key == SDLK_UP ? -1 : 1);
            refresh();
         }
         return true;
      default:
         return false;
   }
}

void ControlsPage::confirm()
{
   readNames();
   const auto playing = _setup.getPlayingColumns();
   if (playing.empty())
   {
      return;
   }

   const auto controllers = _controller_input.getDevices();
   Settings settings(GameSettings::getFilename());
   _setup.store(settings, controllers.size());

   const auto& columns = _setup.getColumns();
   const ControlsSetup::Column& main = columns[playing.front()];
   _client.setPreferredColor(main.color);

   // the main player is already logged in, a new name renames it
   if (!main.name.empty() && main.name != _client.getNick())
   {
      auto* login = GameSettings::getInstance()->getLoginSettings();
      login->setNick(main.name);
      login->serialize();
      _client.rename(main.name);
   }

   _pending_players.clear();
   for (size_t i = 1; i < playing.size(); i++)
   {
      const ControlsSetup::Column& column = columns[playing[i]];
      LocalPlayers::Player player;
      if (column.device.type == ControlsSetup::DeviceType::Controller)
      {
         player.controller = column.device.id;
      }
      player.color = column.color;
      player.nick = column.name;
      _pending_players.push_back(player);
   }

   // controllers left out don't steer anyone
   _pending_excluded.clear();
   for (const auto& info : controllers)
   {
      if (!_setup.findColumn(ControlsSetup::controller(info)))
      {
         _pending_excluded.push_back(info.id);
      }
   }

   if (!_join_connected)
   {
      _join_connection = _client.joinGameResponseSignal.connect([this](bool joined) { joinLocalPlayers(joined); });
      _join_connected = true;
   }
   _client.joinGame(_game_id);
}

void ControlsPage::joinLocalPlayers(bool joined)
{
   if (joined)
   {
      for (const ControllerInput::Id id : _pending_excluded)
      {
         _local_players.exclude(id);
      }
      for (const auto& player : _pending_players)
      {
         _local_players.add(player);
      }
   }

   _pending_players.clear();
   _pending_excluded.clear();
}

void ControlsPage::cancel()
{
   _pending_players.clear();
   _pending_excluded.clear();
   _menu.pageChangeRequest(_return_page);
}
