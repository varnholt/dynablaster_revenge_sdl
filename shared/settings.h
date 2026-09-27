#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class SettingsValue
{
public:
   using SettingsMap = std::map<std::string, std::string>;

   SettingsValue() = default;
   explicit SettingsValue(const std::string& text);

   // raw-pointer out-param mirrors QVariant::toInt(bool*)'s shape; wide external usage
   // (client gamesettings.cpp) makes this a client-subsystem-pass concern, not shared/'s
   int32_t toInt(bool* ok = nullptr) const;
   float toFloat(bool* ok = nullptr) const;
   bool toBool() const;
   std::string toString() const;
   std::vector<std::string> toStringList() const;
   SettingsMap toMap() const;

private:
   std::string _text;
   bool _valid = false;
};

class Settings
{
public:
   using SettingsMap = SettingsValue::SettingsMap;

   enum Format
   {
      IniFormat
   };

   explicit Settings(const std::string& filename, Format format = IniFormat);
   virtual ~Settings();

   void beginGroup(const std::string& group);
   void endGroup();

   std::vector<std::string> childKeys() const;

   SettingsValue value(const std::string& key) const;
   SettingsValue value(const std::string& key, const std::string& default_value) const;
   SettingsValue value(const std::string& key, const char* default_value) const;
   SettingsValue value(const std::string& key, int32_t default_value) const;
   SettingsValue value(const std::string& key, float default_value) const;
   SettingsValue value(const std::string& key, double default_value) const;
   SettingsValue value(const std::string& key, bool default_value) const;

   void setValue(const std::string& key, const std::string& value);
   void setValue(const std::string& key, const char* value);
   void setValue(const std::string& key, int32_t value);
   void setValue(const std::string& key, float value);
   void setValue(const std::string& key, double value);
   void setValue(const std::string& key, bool value);
   void setValue(const std::string& key, const SettingsMap& map);

   void sync();

private:
   struct SharedFile
   {
      std::string filename;
      std::map<std::string, std::string> values;
   };

   std::string qualifiedKey(const std::string& key) const;
   void save() const;

   static std::shared_ptr<SharedFile> acquire(const std::string& filename);

   std::shared_ptr<SharedFile> _file;
   std::vector<std::string> _group_stack;
};
