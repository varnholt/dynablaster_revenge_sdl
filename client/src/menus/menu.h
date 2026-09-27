#pragma once

// shared
#include "settings.h"
#include "signal.h"

// menu
#include "menupage.h"

#include <memory>
#include <vector>

// forward declarations
class MenuWorkflow;

class Menu
{
public:
   Menu();

   Menu(const Menu&);

   virtual ~Menu();

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

   const std::vector<MenuPage*>& getPages() const;

   MenuWorkflow* getMenuWorkflow() const;

   //!
   void setMenuWorkflow(MenuWorkflow*);

   // workflow

   //! action response
   void actionResponse(const std::string& page, const std::string& action, bool ok);

   // workflow

   //! action request
   Signal<const std::string&, const std::string&> actionRequestSignal;

   Signal<MenuPage*, MenuPage*> pageChangeRequestSignal;

   //! a key was pressed while an item was focussed
   Signal<const std::string&, const std::string&, int> actionKeyPressedSignal;

   //! an item was focussed
   Signal<const std::string&, const std::string&> layerFocussedSignal;

private:
   std::vector<MenuPage*> mPages;

   std::unique_ptr<Settings> mSettings;

   MenuPage* mCurrentPage;

   MenuPage* mBackground;

   MenuWorkflow* mMenuWorkflow;

   static Menu* lInstance;
};
