#include "controlssetup.h"

#include "settings.h"

#include <algorithm>

namespace
{
constexpr const char* keyboard_name = "keyboard";
constexpr const char* none_name = "none";

std::string section(size_t controller_count)
{
   return "controls" + std::to_string(controller_count) + "/";
}

Constants::Color colorAt(int32_t index)
{
   return static_cast<Constants::Color>(
      ((index % ControlsSetup::color_count) + ControlsSetup::color_count) % ControlsSetup::color_count + 1
   );
}
}  // namespace

bool ControlsSetup::Device::operator==(const Device& other) const
{
   return type == other.type && (type != DeviceType::Controller || id == other.id);
}

ControlsSetup::Device ControlsSetup::keyboard()
{
   return {DeviceType::Keyboard, 0, {}};
}

ControlsSetup::Device ControlsSetup::controller(const ControllerInput::DeviceInfo& info)
{
   return {DeviceType::Controller, info.id, info.guid};
}

void ControlsSetup::reset(
   const std::vector<ControllerInput::DeviceInfo>& controllers,
   size_t max_columns,
   const std::vector<std::string>& names
)
{
   _devices = {keyboard()};
   for (const auto& info : controllers)
   {
      _devices.push_back(controller(info));
   }

   _columns.clear();
   for (size_t i = 0; i < std::min(_devices.size(), max_columns); i++)
   {
      Column column;
      column.device = _devices[i];
      column.color = colorAt(static_cast<int32_t>(i));
      column.name = i < names.size() ? names[i] : std::string();
      _columns.push_back(column);
   }
}

bool ControlsSetup::restore(Settings& settings, const std::vector<ControllerInput::DeviceInfo>& controllers, size_t max_columns)
{
   const std::string prefix = section(controllers.size());
   const auto count = static_cast<size_t>(settings.value(prefix + "columns", 0).toInt());
   if (count == 0)
   {
      return false;
   }

   std::vector<std::string> names;
   for (size_t i = 0; i < count; i++)
   {
      names.push_back(settings.value(prefix + "column" + std::to_string(i + 1) + "_name").toString());
   }
   reset(controllers, max_columns, names);
   _columns.resize(std::min(count, max_columns));

   // controllers are recognized by their guid; identical models share one, those go in order
   std::vector<Device> unclaimed(_devices.begin() + 1, _devices.end());
   for (size_t i = 0; i < _columns.size(); i++)
   {
      const std::string key = prefix + "column" + std::to_string(i + 1);
      const std::string device = settings.value(key + "_device", none_name).toString();
      Column& column = _columns[i];
      column.name = names[i];
      column.color = colorAt(settings.value(key + "_color", static_cast<int32_t>(i + 1)).toInt() - 1);
      column.device = {};

      if (device == keyboard_name)
      {
         column.device = keyboard();
      }
      else if (device != none_name)
      {
         const auto match = std::ranges::find(unclaimed, device, &Device::guid);
         if (match != unclaimed.end())
         {
            column.device = *match;
            unclaimed.erase(match);
         }
      }
   }

   // keep the keyboard and every controller placeable: unknown ones fill empty columns
   for (const Device& device : _devices)
   {
      if (!findColumn(device) && device.type == DeviceType::Controller && std::ranges::find(unclaimed, device) != unclaimed.end())
      {
         const auto empty = std::ranges::find_if(_columns, [](const Column& column) { return column.device.type == DeviceType::None; });
         if (empty != _columns.end())
         {
            empty->device = device;
         }
      }
   }

   makeColorsUnique();
   return true;
}

void ControlsSetup::store(Settings& settings, size_t controller_count) const
{
   const std::string prefix = section(controller_count);
   settings.setValue(prefix + "columns", static_cast<int32_t>(_columns.size()));
   for (size_t i = 0; i < _columns.size(); i++)
   {
      const Column& column = _columns[i];
      const std::string key = prefix + "column" + std::to_string(i + 1);
      const std::string device = column.device.type == DeviceType::Keyboard     ? std::string(keyboard_name)
                                 : column.device.type == DeviceType::Controller ? column.device.guid
                                                                                : std::string(none_name);
      settings.setValue(key + "_device", device);
      settings.setValue(key + "_color", static_cast<int32_t>(column.color));
      settings.setValue(key + "_name", column.name);
   }
}

bool ControlsSetup::isColorTaken(Constants::Color color, size_t except_column) const
{
   for (size_t i = 0; i < _columns.size(); i++)
   {
      if (i != except_column && _columns[i].device.type != DeviceType::None && _columns[i].color == color)
      {
         return true;
      }
   }
   return false;
}

void ControlsSetup::makeColorsUnique()
{
   for (size_t i = 0; i < _columns.size(); i++)
   {
      if (_columns[i].device.type != DeviceType::None && isColorTaken(_columns[i].color, i))
      {
         cycleColor(i, 1);
      }
   }
}

void ControlsSetup::cycleColor(size_t column, int32_t direction)
{
   if (column >= _columns.size() || direction == 0)
   {
      return;
   }

   const int32_t step = direction > 0 ? 1 : -1;
   int32_t index = static_cast<int32_t>(_columns[column].color) - 1;
   for (int32_t tries = 0; tries < color_count; tries++)
   {
      index += step;
      if (!isColorTaken(colorAt(index), column))
      {
         _columns[column].color = colorAt(index);
         return;
      }
   }
}

void ControlsSetup::cycleDevice(size_t column, int32_t direction)
{
   if (column >= _columns.size() || direction == 0)
   {
      return;
   }

   // none, keyboard, controllers...
   std::vector<Device> choices = {Device{}};
   choices.insert(choices.end(), _devices.begin(), _devices.end());

   const auto current = std::ranges::find(choices, _columns[column].device);
   const auto size = static_cast<int32_t>(choices.size());
   const int32_t index = static_cast<int32_t>(current - choices.begin());
   assign(column, choices[static_cast<size_t>(((index + (direction > 0 ? 1 : -1)) % size + size) % size)]);
}

void ControlsSetup::assign(size_t column, const Device& device)
{
   if (column >= _columns.size())
   {
      return;
   }

   // a device can only be in one column
   if (const auto other = findColumn(device); other && *other != column)
   {
      _columns[*other].device = _columns[column].device;
   }
   _columns[column].device = device;
   makeColorsUnique();
}

void ControlsSetup::moveDevice(size_t column, int32_t direction)
{
   const auto target = static_cast<int64_t>(column) + (direction > 0 ? 1 : -1);
   if (column >= _columns.size() || direction == 0 || target < 0 || target >= static_cast<int64_t>(_columns.size()))
   {
      return;
   }

   std::swap(_columns[column].device, _columns[static_cast<size_t>(target)].device);
   makeColorsUnique();
}

void ControlsSetup::setName(size_t column, const std::string& name)
{
   if (column < _columns.size())
   {
      _columns[column].name = name;
   }
}

const std::vector<ControlsSetup::Column>& ControlsSetup::getColumns() const
{
   return _columns;
}

std::optional<size_t> ControlsSetup::findColumn(const Device& device) const
{
   if (device.type == DeviceType::None)
   {
      return std::nullopt;
   }

   const auto it = std::ranges::find(_columns, device, &Column::device);
   return it != _columns.end() ? std::optional<size_t>(static_cast<size_t>(it - _columns.begin())) : std::nullopt;
}

std::vector<size_t> ControlsSetup::getPlayingColumns() const
{
   std::vector<size_t> playing;
   for (size_t i = 0; i < _columns.size(); i++)
   {
      if (_columns[i].device.type != DeviceType::None)
      {
         playing.push_back(i);
      }
   }
   return playing;
}
