#include "menu.h"

#include <algorithm>
#include <cstdlib>

Menu* Menu::lInstance = 0;

Menu::Menu() : mCurrentPage(0), mBackground(0), mMenuWorkflow(0)
{
   mSettings = std::make_unique<Settings>("data/menus/menu.ini", Settings::IniFormat);

   lInstance = this;
}

Menu::Menu(const Menu& /*menu*/) : mCurrentPage(0), mBackground(0), mMenuWorkflow(0)
{
   std::abort();
}

Menu::~Menu()
{
   for (MenuPage* page : mPages)
   {
      delete page;
   }

   mPages.clear();
}

void Menu::initialize()
{
   // assign 1 psd for each page
   mSettings->beginGroup("pages");
   const auto childKeys = mSettings->childKeys();

   bool defaultAssigned = false;
   for (const auto& childKey : childKeys)
   {
      const auto filename = mSettings->value(childKey).toString();

      // create a new page
      MenuPage* page = new MenuPage();

      page->setTitle(childKey);
      page->setFilename(filename);
      page->initialize();

      // connect page actions to outside world
      page->actionRequestSignal.connect([this](const std::string& p, const std::string& action) { actionRequestSignal(p, action); });

      page->actionKeyPressedSignal.connect([this](const std::string& p, const std::string& item, int key)
                                           { actionKeyPressedSignal(p, item, key); });

      page->layerFocussedSignal.connect([this](const std::string& p, const std::string& item) { layerFocussedSignal(p, item); });

      // store page
      mPages.push_back(page);

      // assign "special" pages
      if (filename.contains("background"))
      {
         mBackground = page;
      }
      else
      {
         if (!defaultAssigned)
         {
            // assign default page
            mCurrentPage = page;
            mCurrentPage->setActive(true);

            // default page is now assigned
            defaultAssigned = true;
         }
      }
   }

   if (mBackground)
   {
      auto it = std::find(mPages.begin(), mPages.end(), mBackground);

      if (it != mPages.end())
      {
         mPages.erase(it);
      }

      mPages.insert(mPages.begin(), mBackground);
   }

   mSettings->endGroup();
}

Menu* Menu::getInstance()
{
   return lInstance;
}

void Menu::mouseMoved(int x, int y)
{
   if (mCurrentPage)
   {
      mCurrentPage->mouseMoved(x, y);
   }
}

void Menu::mousePressed(int x, int y)
{
   if (mCurrentPage)
   {
      mCurrentPage->mousePressed(x, y);
   }
}

void Menu::mouseReleased()
{
   if (mCurrentPage)
   {
      mCurrentPage->mouseReleased();
   }
}

void Menu::keyPressed(int key, const std::string& text)
{
   if (mCurrentPage)
   {
      mCurrentPage->keyPressed(key, text);
   }
}

void Menu::paste(const std::string& text)
{
   if (mCurrentPage)
   {
      mCurrentPage->paste(text);
   }
}

void Menu::setCurrentPage(MenuPage* page)
{
   mCurrentPage = page;
}

MenuPage* Menu::getCurrentPage()
{
   return mCurrentPage;
}

MenuPage* Menu::getBackground()
{
   return mBackground;
}

void Menu::setMenuWorkflow(MenuWorkflow* workflow)
{
   mMenuWorkflow = workflow;
}

void Menu::actionResponse(
   const std::string& /*page*/,
   const std::string& /*action*/,
   bool /*ok*/
)
{
}

MenuPage* Menu::getPageByName(const std::string& pageName)
{
   MenuPage* page = 0;

   for (MenuPage* candidate : mPages)
   {
      if (candidate->getFilename() == pageName)
      {
         page = candidate;
         break;
      }
   }

   return page;
}

const std::vector<MenuPage*>& Menu::getPages() const
{
   return mPages;
}

MenuWorkflow* Menu::getMenuWorkflow() const
{
   return mMenuWorkflow;
}
