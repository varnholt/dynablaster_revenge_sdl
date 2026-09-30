#pragma once

#include "settings.h"
#include "gamesignal.h"

#include "menupage.h"

#include <memory>
#include <vector>

class MenuWorkflow;

class Menu
{
public:
   Menu();
   virtual ~Menu();

   Menu(const Menu&) = delete;
   Menu& operator=(const Menu&) = delete;

   void initialize();

   static Menu* getInstance();

   void mouseMoved(int x, int y);

   void mousePressed(int x, int y);

   void mouseReleased();

   void keyPressed(int key, const std::string&);

   void paste(const std::string& text);

   MenuPage* getCurrentPage();

   void setCurrentPage(MenuPage* page);

   MenuPage* getBackground();

   MenuPage* getPageByName(const std::string&);

   const std::vector<std::unique_ptr<MenuPage>>& getPages() const;

   MenuWorkflow* getMenuWorkflow() const;

   void setMenuWorkflow(MenuWorkflow*);

   //! action response
   void actionResponse(const std::string& page, const std::string& action, bool ok);

   //! action request
   Signal<const std::string&, const std::string&> actionRequestSignal;

   Signal<MenuPage*, MenuPage*> pageChangeRequestSignal;

   //! a key was pressed while an item was focussed
   Signal<const std::string&, const std::string&, int> actionKeyPressedSignal;

   //! an item was focussed
   Signal<const std::string&, const std::string&> layerFocussedSignal;

private:
   std::vector<std::unique_ptr<MenuPage>> _pages;

   std::unique_ptr<Settings> _settings;

   // non-owning, point into _pages
   MenuPage* _current_page = nullptr;
   MenuPage* _background = nullptr;

   MenuWorkflow* _menu_workflow = nullptr;

   static Menu* _instance;
};
