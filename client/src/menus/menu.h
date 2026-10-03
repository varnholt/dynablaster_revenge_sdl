#pragma once

#include "gamesignal.h"
#include "settings.h"

#include "menupage.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class Menu
{
public:
   Menu();
   virtual ~Menu();

   Menu(const Menu&) = delete;
   Menu& operator=(const Menu&) = delete;

   void initialize();

   //! the menu MenuDrawable owns, only while it exists
   static Menu& getInstance();

   void mouseMoved(int x, int y);

   void mousePressed(int x, int y);

   void mouseReleased();

   void keyPressed(int key, const std::string&);

   void paste(const std::string& text);

   std::optional<std::reference_wrapper<MenuPage>> getCurrentPage() const;

   void setCurrentPage(MenuPage& page);

   std::optional<std::reference_wrapper<MenuPage>> getBackground() const;

   std::optional<std::reference_wrapper<MenuPage>> getPageByName(const std::string&) const;

   const std::vector<std::unique_ptr<MenuPage>>& getPages() const;

   //! action response
   void actionResponse(const std::string& page, const std::string& action, bool ok);

   //! action request
   Signal<const std::string&, const std::string&> actionRequestSignal;

   //! a key was pressed while an item was focussed
   Signal<const std::string&, const std::string&, int> actionKeyPressedSignal;

   //! an item was focussed
   Signal<const std::string&, const std::string&> layerFocussedSignal;

private:
   std::vector<std::unique_ptr<MenuPage>> _pages;

   std::unique_ptr<Settings> _settings;

   // point into _pages
   std::optional<std::reference_wrapper<MenuPage>> _current_page;
   std::optional<std::reference_wrapper<MenuPage>> _background;

   static std::optional<std::reference_wrapper<Menu>> _instance;
};
