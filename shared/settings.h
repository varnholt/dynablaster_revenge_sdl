#pragma once

#include <concepts>
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

   int32_t toInt() const;
   float toFloat() const;

   // ok reports whether the conversion succeeded
   int32_t toInt(bool& ok) const;
   float toFloat(bool& ok) const;
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
   SettingsValue value(const std::string& key, int32_t default_value) const;
   SettingsValue value(const std::string& key, float default_value) const;
   SettingsValue value(const std::string& key, double default_value) const;

   // exactly bool, so a string literal default never decays into the bool overload
   template <std::same_as<bool> Bool>
   SettingsValue value(const std::string& key, Bool default_value) const
   {
      return valueBool(key, default_value);
   }

   void setValue(const std::string& key, const std::string& value);
   void setValue(const std::string& key, int32_t value);
   void setValue(const std::string& key, float value);
   void setValue(const std::string& key, double value);

   template <std::same_as<bool> Bool>
   void setValue(const std::string& key, Bool value)
   {
      setValueBool(key, value);
   }

   void setValue(const std::string& key, const SettingsMap& map);

   void sync();

private:
   struct SharedFile
   {
      std::string filename;
      std::map<std::string, std::string> values;
   };

   SettingsValue valueBool(const std::string& key, bool default_value) const;
   void setValueBool(const std::string& key, bool value);

   std::string qualifiedKey(const std::string& key) const;
   void save() const;

   static std::shared_ptr<SharedFile> acquire(const std::string& filename);

   std::shared_ptr<SharedFile> _file;
   std::vector<std::string> _group_stack;
};
