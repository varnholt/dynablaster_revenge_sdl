#pragma once

#include "constants.h"
#include "controllerinput.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Settings;

/// \brief who plays on this machine: one column per player with its input device, color and
/// name. devices and colors are unique across the columns. a setup is remembered per number of
/// connected controllers.
class ControlsSetup
{
public:
   enum class DeviceType
   {
      None,
      Keyboard,
      Controller
   };

   struct Device
   {
      DeviceType type = DeviceType::None;
      ControllerInput::Id id = 0;
      std::string guid;

      bool operator==(const Device& other) const;
   };

   struct Column
   {
      Device device;
      Constants::Color color = Constants::ColorWhite;
      std::string name;
   };

   static constexpr int32_t color_count = 10;

   /// \brief keyboard plus one column per controller (as many as fit), colors in order
   void reset(const std::vector<ControllerInput::DeviceInfo>& controllers, size_t max_columns, const std::vector<std::string>& names);

   /// \brief the remembered setup for this many controllers, false if there is none
   bool restore(Settings& settings, const std::vector<ControllerInput::DeviceInfo>& controllers, size_t max_columns);
   void store(Settings& settings, size_t controller_count) const;

   /// \brief next or previous color not taken by another playing column
   void cycleColor(size_t column, int32_t direction);

   /// \brief next or previous device of all available ones incl. none; one taken by another
   /// column is swapped with it. the last playing column can't sit out.
   void cycleDevice(size_t column, int32_t direction);

   /// \brief cycleDevice() has something to switch to
   bool canCycleDevice(size_t column) const;

   /// \brief puts a device into a column; the column that had it gets this column's device
   void assign(size_t column, const Device& device);

   /// \brief moves a column's device to the neighbouring column, swapping with its device
   void moveDevice(size_t column, int32_t direction);

   void setName(size_t column, const std::string& name);

   const std::vector<Column>& getColumns() const;

   /// \brief the column a device is in
   std::optional<size_t> findColumn(const Device& device) const;

   /// \brief columns with a device, in order; the first one is the main player
   std::vector<size_t> getPlayingColumns() const;

   static Device keyboard();
   static Device controller(const ControllerInput::DeviceInfo& info);

private:
   bool isColorTaken(Constants::Color color, size_t except_column) const;
   std::vector<Device> getDeviceChoices(size_t column) const;
   void makeColorsUnique();

   std::vector<Column> _columns;
   std::vector<Device> _devices;  // keyboard and all connected controllers
};
