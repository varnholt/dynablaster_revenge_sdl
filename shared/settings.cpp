#include "settings.h"

#include "stringutils.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>

namespace
{
constexpr char map_field_separator = static_cast<char>(0x1f);
constexpr char map_record_separator = static_cast<char>(0x1e);

std::vector<std::string> splitKeepEmpty(const std::string& text, char separator)
{
   std::vector<std::string> parts;
   std::string current;

   for (char c : text)
   {
      if (c == separator)
      {
         parts.push_back(current);
         current.clear();
      }
      else
      {
         current += c;
      }
   }

   parts.push_back(current);
   return parts;
}
}  // namespace

SettingsValue::SettingsValue(const std::string& text) : _text(text), _valid(true)
{
}

int32_t SettingsValue::toInt(bool* ok) const
{
   int32_t result = 0;
   const auto [ptr, ec] = std::from_chars(_text.data(), _text.data() + _text.size(), result);
   const auto success = (ec == std::errc()) && ptr == _text.data() + _text.size();
   if (ok)
   {
      *ok = success;
   }
   return success ? result : 0;
}

float SettingsValue::toFloat(bool* ok) const
{
   try
   {
      size_t pos = 0;
      const auto result = std::stof(_text, &pos);
      const auto success = pos == _text.size();
      if (ok)
      {
         *ok = success;
      }
      return success ? result : 0.0f;
   }
   catch (...)
   {
      if (ok)
      {
         *ok = false;
      }
      return 0.0f;
   }
}

bool SettingsValue::toBool() const
{
   return StringUtils::toLower(_text) == "true" || _text == "1";
}

std::string SettingsValue::toString() const
{
   return _text;
}

std::vector<std::string> SettingsValue::toStringList() const
{
   if (_text.empty())
   {
      return {};
   }

   return splitKeepEmpty(_text, ',');
}

SettingsValue::SettingsMap SettingsValue::toMap() const
{
   SettingsMap map;

   const auto records = splitKeepEmpty(_text, map_record_separator);
   for (const auto& record : records)
   {
      const auto fields = splitKeepEmpty(record, map_field_separator);
      if (fields.size() == 2)
      {
         map[fields[0]] = fields[1];
      }
   }

   return map;
}

std::shared_ptr<Settings::SharedFile> Settings::acquire(const std::string& filename)
{
   // process-wide registry so every Settings instance pointed at the same
   // file shares one in-memory state, matching QSettings' own per-path cache -
   // otherwise the last instance destroyed would clobber every other
   // instance's writes with its own load-time snapshot.
   static std::map<std::string, std::weak_ptr<SharedFile>> reg;

   auto it = reg.find(filename);
   if (it != reg.end())
   {
      if (auto existing = it->second.lock())
      {
         return existing;
      }
   }

   auto file = std::make_shared<SharedFile>();
   file->filename = filename;

   std::ifstream io(filename);
   if (io.is_open())
   {
      std::string section;
      std::string line;

      while (std::getline(io, line))
      {
         line = StringUtils::trim(line);

         if (line.empty() || line.front() == '#' || line.front() == ';')
         {
            continue;
         }

         if (line.front() == '[' && line.back() == ']')
         {
            section = line.substr(1, line.size() - 2);
            continue;
         }

         const auto separator_index = line.find('=');
         if (separator_index == std::string::npos)
         {
            continue;
         }

         const auto key = StringUtils::trim(line.substr(0, separator_index));
         const auto value = StringUtils::trim(line.substr(separator_index + 1));

         const auto qualified = section.empty() ? key : section + "/" + key;
         file->values[qualified] = value;
      }
   }

   reg[filename] = file;

   return file;
}

Settings::Settings(const std::string& filename, Format /*format*/) : _file(acquire(filename))
{
}

// setValue() already saves, so a read-only use must not rewrite the file
Settings::~Settings() = default;

void Settings::beginGroup(const std::string& group)
{
   _group_stack.push_back(group);
}

void Settings::endGroup()
{
   if (!_group_stack.empty())
   {
      _group_stack.pop_back();
   }
}

std::string Settings::qualifiedKey(const std::string& key) const
{
   if (_group_stack.empty())
   {
      return key;
   }

   std::string joined;
   for (const auto& group : _group_stack)
   {
      if (!joined.empty())
      {
         joined += "/";
      }
      joined += group;
   }

   return joined + "/" + key;
}

std::vector<std::string> Settings::childKeys() const
{
   const auto prefix = qualifiedKey(std::string());
   std::vector<std::string> keys;

   for (const auto& entry : _file->values)
   {
      if (entry.first.size() < prefix.size() || entry.first.compare(0, prefix.size(), prefix) != 0)
      {
         continue;
      }

      const auto remainder = entry.first.substr(prefix.size());
      if (remainder.find('/') == std::string::npos)
      {
         keys.push_back(remainder);
      }
   }

   return keys;
}

SettingsValue Settings::value(const std::string& key) const
{
   const auto& values = _file->values;
   const auto it = values.find(qualifiedKey(key));
   if (it == values.end())
   {
      return SettingsValue();
   }

   return SettingsValue(it->second);
}

SettingsValue Settings::value(const std::string& key, const std::string& default_value) const
{
   const auto& values = _file->values;
   const auto it = values.find(qualifiedKey(key));
   return SettingsValue(it == values.end() ? default_value : it->second);
}

SettingsValue Settings::value(const std::string& key, const char* default_value) const
{
   return value(key, std::string(default_value));
}

SettingsValue Settings::value(const std::string& key, int32_t default_value) const
{
   const auto& values = _file->values;
   const auto it = values.find(qualifiedKey(key));
   return SettingsValue(it == values.end() ? std::to_string(default_value) : it->second);
}

SettingsValue Settings::value(const std::string& key, float default_value) const
{
   const auto& values = _file->values;
   const auto it = values.find(qualifiedKey(key));
   return SettingsValue(it == values.end() ? std::to_string(default_value) : it->second);
}

SettingsValue Settings::value(const std::string& key, double default_value) const
{
   const auto& values = _file->values;
   const auto it = values.find(qualifiedKey(key));
   return SettingsValue(it == values.end() ? std::to_string(default_value) : it->second);
}

SettingsValue Settings::value(const std::string& key, bool default_value) const
{
   const auto& values = _file->values;
   const auto it = values.find(qualifiedKey(key));
   return SettingsValue(it == values.end() ? std::string(default_value ? "true" : "false") : it->second);
}

void Settings::setValue(const std::string& key, const std::string& value)
{
   _file->values[qualifiedKey(key)] = value;
   save();
}

void Settings::setValue(const std::string& key, const char* value)
{
   setValue(key, std::string(value));
}

void Settings::setValue(const std::string& key, int32_t value)
{
   setValue(key, std::to_string(value));
}

void Settings::setValue(const std::string& key, float value)
{
   setValue(key, std::to_string(value));
}

void Settings::setValue(const std::string& key, double value)
{
   setValue(key, std::to_string(value));
}

void Settings::setValue(const std::string& key, bool value)
{
   setValue(key, std::string(value ? "true" : "false"));
}

void Settings::setValue(const std::string& key, const SettingsMap& map)
{
   std::string text;

   for (const auto& entry : map)
   {
      if (!text.empty())
      {
         text += map_record_separator;
      }

      text += entry.first;
      text += map_field_separator;
      text += entry.second;
   }

   setValue(key, text);
}

void Settings::sync()
{
   save();
}

void Settings::save() const
{
   std::ofstream io(_file->filename);
   if (!io.is_open())
   {
      return;
   }

   std::map<std::string, std::vector<std::pair<std::string, std::string>>> sections;

   for (const auto& entry : _file->values)
   {
      const auto separator_index = entry.first.rfind('/');

      std::string section;
      std::string key = entry.first;

      if (separator_index != std::string::npos)
      {
         section = entry.first.substr(0, separator_index);
         key = entry.first.substr(separator_index + 1);
      }

      sections[section].emplace_back(key, entry.second);
   }

   for (const auto& section_entry : sections)
   {
      if (!section_entry.first.empty())
      {
         io << "[" << section_entry.first << "]\n";
      }

      for (const auto& key_value : section_entry.second)
      {
         io << key_value.first << "=" << key_value.second << "\n";
      }

      io << "\n";
   }
}
