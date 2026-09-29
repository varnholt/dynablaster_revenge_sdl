#include "menu.h"

#include <algorithm>

Menu* Menu::_instance = nullptr;

Menu::Menu()
{
   _settings = std::make_unique<Settings>("data/menus/menu.ini", Settings::IniFormat);

   _instance = this;
}

Menu::~Menu() = default;

void Menu::initialize()
{
   // assign 1 psd for each page
   _settings->beginGroup("pages");
   const auto child_keys = _settings->childKeys();

   bool default_assigned = false;
   for (const auto& child_key : child_keys)
   {
      const auto filename = _settings->value(child_key).toString();

      auto page = std::make_unique<MenuPage>();

      page->setTitle(child_key);
      page->setFilename(filename);
      page->initialize();

      // connect page actions to outside world; the pages are owned by this menu
      page->actionRequestSignal.connect([this](const std::string& page_name, const std::string& action)
                                        { actionRequestSignal(page_name, action); });

      page->actionKeyPressedSignal.connect([this](const std::string& page_name, const std::string& item, int key)
                                           { actionKeyPressedSignal(page_name, item, key); });

      page->layerFocussedSignal.connect([this](const std::string& page_name, const std::string& item)
                                        { layerFocussedSignal(page_name, item); });

      // assign "special" pages
      if (filename.contains("background"))
      {
         _background = page.get();
      }
      else if (!default_assigned)
      {
         _current_page = page.get();
         _current_page->setActive(true);
         default_assigned = true;
      }

      _pages.push_back(std::move(page));
   }

   // the background page is always rendered first
   if (_background)
   {
      const auto background = std::ranges::find_if(_pages, [this](const auto& page) { return page.get() == _background; });

      if (background != _pages.end())
      {
         std::rotate(_pages.begin(), background, background + 1);
      }
   }

   _settings->endGroup();
}

Menu* Menu::getInstance()
{
   return _instance;
}

void Menu::mouseMoved(int x, int y)
{
   if (_current_page)
   {
      _current_page->mouseMoved(x, y);
   }
}

void Menu::mousePressed(int x, int y)
{
   if (_current_page)
   {
      _current_page->mousePressed(x, y);
   }
}

void Menu::mouseReleased()
{
   if (_current_page)
   {
      _current_page->mouseReleased();
   }
}

void Menu::keyPressed(int key, const std::string& text)
{
   if (_current_page)
   {
      _current_page->keyPressed(key, text);
   }
}

void Menu::paste(const std::string& text)
{
   if (_current_page)
   {
      _current_page->paste(text);
   }
}

void Menu::setCurrentPage(MenuPage* page)
{
   _current_page = page;
}

MenuPage* Menu::getCurrentPage()
{
   return _current_page;
}

MenuPage* Menu::getBackground()
{
   return _background;
}

void Menu::setMenuWorkflow(MenuWorkflow* workflow)
{
   _menu_workflow = workflow;
}

void Menu::actionResponse(const std::string& /*page*/, const std::string& /*action*/, bool /*ok*/)
{
}

MenuPage* Menu::getPageByName(const std::string& page_name)
{
   const auto iterator = std::ranges::find_if(_pages, [&page_name](const auto& page) { return page->getFilename() == page_name; });
   return iterator != _pages.end() ? iterator->get() : nullptr;
}

const std::vector<std::unique_ptr<MenuPage>>& Menu::getPages() const
{
   return _pages;
}

MenuWorkflow* Menu::getMenuWorkflow() const
{
   return _menu_workflow;
}
