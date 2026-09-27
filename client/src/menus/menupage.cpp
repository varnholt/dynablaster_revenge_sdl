
// header
#include "menupage.h"
#include "psdlayer.h"

// menu
#include "menupageanimation.h"
#include "menupagebackgrounditem.h"
#include "menupagebuttonitem.h"
#include "menupagecheckboxitem.h"
#include "menupagecomboboxitem.h"
#include "menupageeditablecomboboxitem.h"
#include "menupagelabelitem.h"
#include "menupagelistitem.h"
#include "menupagepixmapitem.h"
#include "menupagescrollbar.h"
#include "menupagescrollimageitem.h"
#include "menupageslideritem.h"
#include "menupagetextedit.h"

// shared
#include "stringutils.h"

#include <SDL3/SDL_keycode.h>

namespace
{
void eraseAll(std::string& str, const std::string& sub)
{
   size_t pos = 0;
   while ((pos = str.find(sub, pos)) != std::string::npos)
   {
      str.erase(pos, sub.length());
   }
}

std::vector<std::string> splitByUnderscore(const std::string& s)
{
   std::vector<std::string> result;
   size_t start = 0;
   size_t pos = s.find('_');

   while (pos != std::string::npos)
   {
      result.push_back(s.substr(start, pos - start));
      start = pos + 1;
      pos = s.find('_', start);
   }

   result.push_back(s.substr(start));

   return result;
}
}  // namespace

MenuPage::MenuPage() : PSD(), mActiveItem(0), mAnimation(0), mActive(false)
{
}

MenuPage::~MenuPage()
{
   for (MenuPageItem* item : mPageItems)
      delete item;
   mPageItems.clear();

   for (PSDLayer* layer : mRenderLayers)
      delete layer;
   mRenderLayers.clear();
}

void MenuPage::initialize()
{
   // init settings
   mSettings = std::make_unique<Settings>("data/menus/menu.ini", Settings::IniFormat);

   // initialize layer information
   initializeLayers();

   // initialize page items
   initializePageItems();

   // initialize tab indices
   initializeTabIndices();
}

void MenuPage::initializeLayers()
{
   load(mFilename.c_str());

   // assign layers to menu page items
   for (int l = 0; l < getLayerCount(); l++)
   {
      PSD::Layer* layer = getLayer(l);

      std::string layerName = layer->getName();
      PSDLayer* renderLayer = 0;
      if (layerName.starts_with("background"))
         renderLayer = new PSDLayer(layer, -1.0f, false);
      else
         renderLayer = new PSDLayer(layer, -1.0f);

      mRenderLayers.push_back(renderLayer);
   }
}

MenuPageItem* MenuPage::processLabel(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageLabelItem();
   mPageItems.push_back(pageItem);

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   std::string fontNameKey = layerName + "_font_name";
   std::string fontXOffsetKey = layerName + "_font_x_offset";
   std::string fontYOffsetKey = layerName + "_font_y_offset";
   std::string maxCharsKey = layerName + "_field_width";
   std::string scaleKey = layerName + "_scale";
   std::string colorKey = layerName + "_color";
   std::string alphaKey = layerName + "_alpha";

   std::string fontName = mSettings->value(fontNameKey, "default").toString();
   int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
   int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
   int maxChars = mSettings->value(maxCharsKey).toInt();
   float scale = mSettings->value(scaleKey).toFloat();
   Color color = Color(mSettings->value(colorKey, "#FFFFFF").toString());
   int alpha = mSettings->value(alphaKey, 255).toInt();

   // set label properties
   ((MenuPageLabelItem*)pageItem)->setFontName(fontName);
   ((MenuPageLabelItem*)pageItem)->setFontXOffset(fontXOffset);
   ((MenuPageLabelItem*)pageItem)->setFontYOffset(fontYOffset);
   ((MenuPageLabelItem*)pageItem)->setMaxChars(maxChars);
   ((MenuPageLabelItem*)pageItem)->setScale(scale);
   ((MenuPageLabelItem*)pageItem)->setColor(color);
   ((MenuPageLabelItem*)pageItem)->setAlpha(alpha);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   return pageItem;
}

MenuPageItem* MenuPage::processLineEdit(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageTextEditItem();
   mPageItems.push_back(pageItem);

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   std::string fontNameKey = layerName + "_font_name";
   std::string fontXOffsetKey = layerName + "_font_x_offset";
   std::string fontYOffsetKey = layerName + "_font_y_offset";
   std::string fieldWidthKey = layerName + "_field_width";
   std::string fieldMaxLength = layerName + "_max_length";
   std::string scaleKey = layerName + "_scale";
   std::string colorKey = layerName + "_color";
   std::string alphaKey = layerName + "_alpha";

   std::string fontName = mSettings->value(fontNameKey, "default").toString();
   int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
   int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
   int fieldWidth = mSettings->value(fieldWidthKey).toInt();
   int maxLength = mSettings->value(fieldMaxLength).toInt();
   float scale = mSettings->value(scaleKey).toFloat();
   Color color = Color(mSettings->value(colorKey, "#FFFFFF").toString());
   int alpha = mSettings->value(alphaKey, 255).toInt();

   // set lineedit properties
   ((MenuPageTextEditItem*)pageItem)->setFontName(fontName);
   ((MenuPageTextEditItem*)pageItem)->setFontXOffset(fontXOffset);
   ((MenuPageTextEditItem*)pageItem)->setFontYOffset(fontYOffset);
   ((MenuPageTextEditItem*)pageItem)->setFieldWidth(fieldWidth);
   ((MenuPageTextEditItem*)pageItem)->setMaxLength(maxLength);
   ((MenuPageTextEditItem*)pageItem)->setScale(scale);
   ((MenuPageTextEditItem*)pageItem)->setColor(color);
   ((MenuPageTextEditItem*)pageItem)->setAlpha(alpha);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   return pageItem;
}

MenuPageItem* MenuPage::processBackground(PSDLayer* layer, std::string layerName)
{
   bool added = false;
   MenuPageItem* pageItem = 0;

   std::string baseName = "background_" + (splitByUnderscore(layerName).at(1));

   if (!mPageItemNameMap.contains(baseName))
   {
      added = true;
      pageItem = new MenuPageBackgroundItem();
      mPageItems.push_back(pageItem);
      mPageItemNameMap[baseName] = pageItem;
   }
   else
   {
      pageItem = mPageItemNameMap[baseName];
   }

   if (layerName.ends_with("_active"))
   {
      pageItem->setActiveLayer(layer);
      pageItem->setInactiveLayer(layer);
   }
   else if (layerName.contains("_gradient"))
   {
      MenuPageBackgroundItem::BackgroundColor color;

      if (layerName.ends_with("_red"))
         color = MenuPageBackgroundItem::BackgroundColorRed;
      else if (layerName.ends_with("_green"))
         color = MenuPageBackgroundItem::BackgroundColorGreen;
      else
         color = MenuPageBackgroundItem::BackgroundColorBlue;

      dynamic_cast<MenuPageBackgroundItem*>(pageItem)->addGradientLayer(layer, color);
   }

   return added ? pageItem : 0;
}

MenuPageItem* MenuPage::processTableMain(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageListItem();
   mPageItems.push_back(pageItem);

   std::string baseName = layerName;
   eraseAll(baseName, "_main");

   // read lineedit properties
   std::string fontNameKey = baseName + "_font_name";
   std::string fontXOffsetKey = baseName + "_font_x_offset";
   std::string fontYOffsetKey = baseName + "_font_y_offset";
   std::string fieldWidthKey = baseName + "_field_width";
   std::string scaleKey = baseName + "_scale";
   std::string rowHeightKey = baseName + "_row_height";

   std::string fontName = mSettings->value(fontNameKey, "default").toString();
   int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
   int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
   int fieldWidth = mSettings->value(fieldWidthKey).toInt();
   float scale = mSettings->value(scaleKey).toFloat();
   int rowHeight = mSettings->value(rowHeightKey).toInt();

   // set lineedit properties
   ((MenuPageListItem*)pageItem)->setFontName(fontName);
   ((MenuPageListItem*)pageItem)->setFontXOffset(fontXOffset);
   ((MenuPageListItem*)pageItem)->setFontYOffset(fontYOffset);
   ((MenuPageListItem*)pageItem)->setFieldWidth(fieldWidth);
   ((MenuPageListItem*)pageItem)->setScale(scale);
   ((MenuPageListItem*)pageItem)->setRowHeight(rowHeight);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   return pageItem;
}

MenuPageItem* MenuPage::processTableScrollButtons(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageItem();
   mPageItems.push_back(pageItem);

   pageItem->setInteractive(true);

   bool up = false;

   if (layerName.contains("scroll_up"))
   {
      up = true;
      pageItem->setAction("scroll_up");
   }
   else
   {
      pageItem->setAction("scroll_down");
   }

   std::string baseLayer = "table_" + (splitByUnderscore(layerName).at(1)) + "_main";

   MenuPageListItem* scrollTarget = (MenuPageListItem*)mPageItemNameMap[baseLayer];

   if (up)
   {
      pageItem->actionSignal.connect([scrollTarget](const std::string&) { scrollTarget->scrollUp(); });
   }
   else
   {
      pageItem->actionSignal.connect([scrollTarget](const std::string&) { scrollTarget->scrollDown(); });
   }

   pageItem->mouseReleasedSignal.connect([scrollTarget]() { scrollTarget->scrollStop(); });

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   return pageItem;
}

MenuPageItem* MenuPage::processTableScrollBar(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageItem();
   mPageItems.push_back(pageItem);

   pageItem->setInteractive(true);

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   return pageItem;
}

MenuPageItem* MenuPage::processTableScrollBarSlider(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageScrollbar();
   mPageItems.push_back(pageItem);

   pageItem->setInteractive(true);

   std::string scrollAreaLayer = "table_" + (splitByUnderscore(layerName).at(1)) + "_scrollbar";

   // connect slider to table
   std::string baseLayer = "table_" + (splitByUnderscore(layerName).at(1)) + "_main";

   MenuPageListItem* tableItem = (MenuPageListItem*)mPageItemNameMap[baseLayer];
   MenuPageScrollbar* scrollbarItem = (MenuPageScrollbar*)pageItem;

   scrollbarItem->scrollToPercentageSignal.connect([tableItem](float percent) { tableItem->scrollToPercentage(percent); });

   MenuPageItem* scrollbar = mPageItemNameMap[scrollAreaLayer];
   scrollbarItem->setTop(scrollbar->getCurrentLayer()->getTop());
   scrollbarItem->setHeight(scrollbar->getCurrentLayer()->getHeight());

   // connect table back to slider
   tableItem->scrollAnimationSignal.connect([scrollbarItem](float percent) { scrollbarItem->updateFromAnimation(percent); });

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   return pageItem;
}

MenuPageItem* MenuPage::processSliderScrollBarIcons(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   pageItem = new MenuPageSliderItem();
   mPageItems.push_back(pageItem);
   mPageItemNameMap[layerName] = pageItem;

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   for (int l = 0; l < getLayerCount(); l++)
   {
      PSD::Layer* tmpLayer = getLayer(l);
      if (tmpLayer->getName() == (layerName) + "_bar")
      {
         ((MenuPageSliderItem*)pageItem)->setMinimum(tmpLayer->getLeft());

         ((MenuPageSliderItem*)pageItem)->setMaximum(tmpLayer->getLeft() + tmpLayer->getWidth());
      }
   }

   return pageItem;
}

MenuPageItem* MenuPage::processScrollImage(PSDLayer* layer, std::string layerName)
{
   std::string baseName;
   MenuPageScrollImageItem* sci = 0;
   bool clipRect = false;
   bool complete = false;

   // determine basename
   //
   // image_scroll_cliprect
   // image_scroll
   if (layerName.contains("_cliprect"))
   {
      baseName = layerName;
      eraseAll(baseName, "_cliprect");
      clipRect = true;
   }
   else
   {
      baseName = layerName;
   }

   // look up item
   if (mPageItemNameMap.contains(baseName))
   {
      sci = dynamic_cast<MenuPageScrollImageItem*>(mPageItemNameMap[baseName]);

      // page item is now complete and can be initialized
      complete = true;
   }
   else
   {
      sci = new MenuPageScrollImageItem();
      mPageItemNameMap[baseName] = sci;
      mPageItems.push_back(sci);
   }

   if (clipRect)
   {
      // we use the inactive layer as clipping layer
      sci->setInactiveLayer(layer);
   }
   else
   {
      sci->setActiveLayer(layer);
   }

   return complete ? sci : 0;
}

MenuPageItem* MenuPage::processCheckBox(PSDLayer* layer, std::string layerNameWithoutPostfix, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   layerNameWithoutPostfix = layerName;
   eraseAll(layerNameWithoutPostfix, "_yes");
   eraseAll(layerNameWithoutPostfix, "_no");

   // find page item
   if (!mPageItemNameMap.contains(layerNameWithoutPostfix))
   {
      // create a new page item
      pageItem = new MenuPageCheckBoxItem();

      // store button action
      pageItem->setAction(mSettings->value(layerNameWithoutPostfix).toString());

      mPageItems.push_back(pageItem);

      // store page item without postfix names
      mPageItemNameMap[layerNameWithoutPostfix] = pageItem;
   }
   else
   {
      // use previously assigned pageitem
      pageItem = mPageItemNameMap[layerNameWithoutPostfix];
   }

   // store
   if (layerName.ends_with("_no"))
   {
      dynamic_cast<MenuPageCheckBoxItem*>(pageItem)->setUncheckedLayer(layer);
   }
   else if (layerName.ends_with("_yes"))
   {
      dynamic_cast<MenuPageCheckBoxItem*>(pageItem)->setCheckedLayer(layer);
   }

   return pageItem;
}

MenuPageItem* MenuPage::processPixmap(PSDLayer* layer, std::string layerName)
{
   MenuPagePixmapItem* pageItem = new MenuPagePixmapItem();
   mPageItems.push_back(pageItem);

   // both layers are the same
   pageItem->setActiveLayer(layer);
   pageItem->setInactiveLayer(layer);

   // store page item without postfix names
   mPageItemNameMap[layerName] = pageItem;

   return pageItem;
}

MenuPageItem* MenuPage::processDefaultItem(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;

   if (!layerName.starts_with("unused_"))
   {
      pageItem = new MenuPageItem();
      mPageItems.push_back(pageItem);

      // both layers are the same
      pageItem->setActiveLayer(layer);
      pageItem->setInactiveLayer(layer);

      // store page item without postfix names
      mPageItemNameMap[layerName] = pageItem;
   }

   return pageItem;
}

MenuPageItem* MenuPage::processButton(PSDLayer* layer, std::string layerName, std::string layerNameWithoutPostfix)
{
   MenuPageItem* pageItem = 0;
   layerNameWithoutPostfix = layerName;
   eraseAll(layerNameWithoutPostfix, "_inactive");
   eraseAll(layerNameWithoutPostfix, "_active");

   // find page item
   if (!mPageItemNameMap.contains(layerNameWithoutPostfix))
   {
      // create a new page item
      pageItem = new MenuPageButtonItem();

      // store button action
      pageItem->setAction(mSettings->value(layerNameWithoutPostfix).toString());

      pageItem->actionSignal.connect([this](const std::string& action) { actionRequestFromItem(action); });

      mPageItems.push_back(pageItem);

      // store page item without postfix names
      mPageItemNameMap[layerNameWithoutPostfix] = pageItem;
   }
   else
   {
      // use previously assigned pageitem
      pageItem = mPageItemNameMap[layerNameWithoutPostfix];
   }

   // store
   if (layerName.ends_with("_inactive"))
   {
      pageItem->setInactiveLayer(layer);
   }

   else if (layerName.ends_with("_active"))
   {
      pageItem->setActiveLayer(layer);
   }

   return pageItem;
}

MenuPageItem* MenuPage::processComboBox(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   std::vector<std::string> items = splitByUnderscore(layerName);
   std::string itemName = items.at(1);

   // - button
   // - label
   // - table
   std::string itemType = items.at(2);

   std::string prefix = "combobox_" + (itemName) + "_";
   eraseAll(layerName, prefix);
   std::string postfix = layerName;

   // - combobox_YYY_button
   // - combobox_YYY_table
   std::string baseName = (prefix) + (itemType);
   std::string baseNameButton = (prefix) + "button";
   std::string baseNameTable = (prefix) + "table";
   std::string baseNameLabel = (prefix) + "label";

   // if layername ends with table
   if (StringUtils::toLower(itemType) == "table")
   {
      // if menupageitem not yet created
      if (!mPageItemNameMap.contains(baseName))
      {
         // combobox item
         pageItem = new MenuPageComboBoxItem();
         mPageItems.push_back(pageItem);

         MenuPageComboBoxItem::addComboBox(baseName, (MenuPageComboBoxItem*)pageItem);
         MenuPageComboBoxItem::linkComboBoxToButton(baseNameButton, baseNameTable);

         // read lineedit properties
         std::string fontNameKey = (baseName) + "_font_name";
         std::string fontXOffsetKey = (baseName) + "_font_x_offset";
         std::string fontYOffsetKey = (baseName) + "_font_y_offset";
         std::string scaleKey = (baseName) + "_scale";
         std::string rowHeightKey = (baseName) + "_row_height";

         std::string fontName = mSettings->value(fontNameKey, "default").toString();
         int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
         int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
         float scale = mSettings->value(scaleKey).toFloat();
         int rowHeight = mSettings->value(rowHeightKey).toInt();

         // set lineedit properties
         ((MenuPageListItem*)pageItem)->setFontName(fontName);
         ((MenuPageListItem*)pageItem)->setFontXOffset(fontXOffset);
         ((MenuPageListItem*)pageItem)->setFontYOffset(fontYOffset);
         ((MenuPageListItem*)pageItem)->setScale(scale);
         ((MenuPageListItem*)pageItem)->setRowHeight(rowHeight);

         // store page item without postfix names
         mPageItemNameMap[baseName] = pageItem;

         // if link between combobox table and button not yet made
         MenuPageComboBoxItem::addComboBox(baseName, (MenuPageComboBoxItem*)pageItem);
         MenuPageComboBoxItem::linkComboBoxToButton(baseNameButton, baseNameTable);
      }
      else
      {
         pageItem = mPageItemNameMap[baseName];
      }

      if (postfix == "table_selected_item")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerSelectedElement(layer);
      }
      else if (postfix == "table_focussed_item")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerFocussedElement(layer);
      }
      else if (postfix == "table_bg_first")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerFirstElement(layer);
      }
      else if (postfix == "table_bg_last")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerLastElement(layer);
      }
      else if (postfix == "table_bg_default")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerDefaultElement(layer);
      }
      else if (postfix == "table_gradient")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerGradientElement(layer);

         // maybe the gradient layer is good enough as active/inactive layer
         pageItem->setActiveLayer(layer);
         pageItem->setInactiveLayer(layer);
      }
   }

   // if layername ends with button
   else if (StringUtils::toLower(itemType) == "button")
   {
      // create a standard button

      MenuPageItem* pageItem = 0;

      // find page item
      if (!mPageItemNameMap.contains(baseName))
      {
         // create a new page item
         pageItem = new MenuPageButtonItem();

         // store button action
         pageItem->setAction(mSettings->value(baseName).toString());

         pageItem->actionSignal.connect([this](const std::string& action) { actionRequestFromItem(action); });

         mPageItems.push_back(pageItem);

         // store page item without postfix names
         mPageItemNameMap[baseName] = pageItem;
      }
      else
      {
         // use previously assigned pageitem
         pageItem = mPageItemNameMap[baseName];
      }

      // store
      if (layerName.ends_with("_inactive"))
      {
         pageItem->setInactiveLayer(layer);
      }

      else if (layerName.ends_with("_active"))
      {
         pageItem->setActiveLayer(layer);
      }

      // if link between combobox table and button not yet made
      MenuPageComboBoxItem::addButton(baseName, (MenuPageButtonItem*)pageItem);
      MenuPageComboBoxItem::linkComboBoxToButton(baseNameButton, baseNameTable);
   }

   // if layername ends with label
   else if (StringUtils::toLower(itemType) == "label")
   {
      pageItem = new MenuPageLabelItem();
      mPageItems.push_back(pageItem);

      // both layers are the same
      pageItem->setActiveLayer(layer);
      pageItem->setInactiveLayer(layer);

      std::string fontNameKey = (baseName) + "_font_name";
      std::string fontXOffsetKey = (baseName) + "_font_x_offset";
      std::string fontYOffsetKey = (baseName) + "_font_y_offset";
      std::string scaleKey = (baseName) + "_scale";

      std::string fontName = mSettings->value(fontNameKey, "default").toString();
      int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
      int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
      float scale = mSettings->value(scaleKey).toFloat();

      // set label properties
      ((MenuPageLabelItem*)pageItem)->setFontName(fontName);
      ((MenuPageLabelItem*)pageItem)->setFontXOffset(fontXOffset);
      ((MenuPageLabelItem*)pageItem)->setFontYOffset(fontYOffset);
      ((MenuPageLabelItem*)pageItem)->setScale(scale);

      // store page item without postfix names
      mPageItemNameMap[baseName] = pageItem;

      // if link between combobox table and button not yet made
      MenuPageComboBoxItem::addLabel(baseName, (MenuPageLabelItem*)pageItem);
      MenuPageComboBoxItem::linkComboBoxToLabel(baseNameLabel, baseNameTable);
   }

   return pageItem;
}

MenuPageItem* MenuPage::processEditableComboBox(PSDLayer* layer, std::string layerName)
{
   MenuPageItem* pageItem = 0;
   std::vector<std::string> items = splitByUnderscore(layerName);
   std::string itemName = items.at(1);

   // - button
   // - label
   // - table
   std::string itemType = items.at(2);

   std::string prefix = "editablecombobox_" + (itemName) + "_";
   eraseAll(layerName, prefix);
   std::string postfix = layerName;

   // - combobox_YYY_button
   // - combobox_YYY_table
   std::string baseName = (prefix) + (itemType);
   std::string baseNameButton = (prefix) + "button";
   std::string baseNameTable = (prefix) + "table";
   std::string baseNameLineEdit = (prefix) + "lineedit";

   // if layername ends with table
   if (StringUtils::toLower(itemType) == "table")
   {
      // if menupageitem not yet created
      if (!mPageItemNameMap.contains(baseName))
      {
         // combobox item
         pageItem = new MenuPageEditableComboBoxItem();
         mPageItems.push_back(pageItem);

         MenuPageComboBoxItem::addComboBox(baseName, (MenuPageComboBoxItem*)pageItem);
         MenuPageComboBoxItem::linkComboBoxToButton(baseNameButton, baseNameTable);

         // read lineedit properties
         std::string fontNameKey = (baseName) + "_font_name";
         std::string fontXOffsetKey = (baseName) + "_font_x_offset";
         std::string fontYOffsetKey = (baseName) + "_font_y_offset";
         std::string scaleKey = (baseName) + "_scale";
         std::string rowHeightKey = (baseName) + "_row_height";

         std::string fontName = mSettings->value(fontNameKey, "default").toString();
         int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
         int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
         float scale = mSettings->value(scaleKey).toFloat();
         int rowHeight = mSettings->value(rowHeightKey).toInt();

         // set lineedit properties
         ((MenuPageListItem*)pageItem)->setFontName(fontName);
         ((MenuPageListItem*)pageItem)->setFontXOffset(fontXOffset);
         ((MenuPageListItem*)pageItem)->setFontYOffset(fontYOffset);
         ((MenuPageListItem*)pageItem)->setScale(scale);
         ((MenuPageListItem*)pageItem)->setRowHeight(rowHeight);

         // store page item without postfix names
         mPageItemNameMap[baseName] = pageItem;

         // if link between combobox table and button not yet made
         MenuPageComboBoxItem::addComboBox(baseName, (MenuPageComboBoxItem*)pageItem);
         MenuPageComboBoxItem::linkComboBoxToButton(baseNameButton, baseNameTable);
      }
      else
      {
         pageItem = mPageItemNameMap[baseName];
      }

      if (postfix == "table_selected_item")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerSelectedElement(layer);
      }
      else if (postfix == "table_focussed_item")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerFocussedElement(layer);
      }
      else if (postfix == "table_bg_first")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerFirstElement(layer);
      }
      else if (postfix == "table_bg_last")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerLastElement(layer);
      }
      else if (postfix == "table_bg_default")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerDefaultElement(layer);
      }
      else if (postfix == "table_gradient")
      {
         ((MenuPageComboBoxItem*)pageItem)->setLayerGradientElement(layer);

         // maybe the gradient layer is good enough as active/inactive layer
         pageItem->setActiveLayer(layer);
         pageItem->setInactiveLayer(layer);
      }
   }

   // if layername ends with button
   else if (StringUtils::toLower(itemType) == "button")
   {
      // create a standard button

      MenuPageItem* pageItem = 0;

      // find page item
      if (!mPageItemNameMap.contains(baseName))
      {
         // create a new page item
         pageItem = new MenuPageButtonItem();

         // store button action
         pageItem->setAction(mSettings->value(baseName).toString());

         pageItem->actionSignal.connect([this](const std::string& action) { actionRequestFromItem(action); });

         mPageItems.push_back(pageItem);

         // store page item without postfix names
         mPageItemNameMap[baseName] = pageItem;
      }
      else
      {
         // use previously assigned pageitem
         pageItem = mPageItemNameMap[baseName];
      }

      // store
      if (layerName.ends_with("_inactive"))
      {
         pageItem->setInactiveLayer(layer);
      }

      else if (layerName.ends_with("_active"))
      {
         pageItem->setActiveLayer(layer);
      }

      // if link between combobox table and button not yet made
      MenuPageComboBoxItem::addButton(baseName, (MenuPageButtonItem*)pageItem);
      MenuPageComboBoxItem::linkComboBoxToButton(baseNameButton, baseNameTable);
   }

   // if layername ends with lineedit
   else if (StringUtils::toLower(itemType) == "lineedit")
   {
      pageItem = new MenuPageTextEditItem();
      mPageItems.push_back(pageItem);

      // both layers are the same
      pageItem->setActiveLayer(layer);
      pageItem->setInactiveLayer(layer);

      std::string fontNameKey = (baseName) + "_font_name";
      std::string fontXOffsetKey = (baseName) + "_font_x_offset";
      std::string fontYOffsetKey = (baseName) + "_font_y_offset";
      std::string fieldWidthKey = (baseName) + "_field_width";
      std::string fieldMaxLength = (baseName) + "_max_length";
      std::string scaleKey = (baseName) + "_scale";
      std::string colorKey = (baseName) + "_color";
      std::string alphaKey = (baseName) + "_alpha";

      std::string fontName = mSettings->value(fontNameKey, "default").toString();
      int fontXOffset = mSettings->value(fontXOffsetKey).toInt();
      int fontYOffset = mSettings->value(fontYOffsetKey).toInt();
      int fieldWidth = mSettings->value(fieldWidthKey).toInt();
      int maxLength = mSettings->value(fieldMaxLength).toInt();
      float scale = mSettings->value(scaleKey).toFloat();
      Color color = Color(mSettings->value(colorKey, "#FFFFFF").toString());
      int alpha = mSettings->value(alphaKey, 255).toInt();

      // set lineedit properties
      ((MenuPageTextEditItem*)pageItem)->setFontName(fontName);
      ((MenuPageTextEditItem*)pageItem)->setFontXOffset(fontXOffset);
      ((MenuPageTextEditItem*)pageItem)->setFontYOffset(fontYOffset);
      ((MenuPageTextEditItem*)pageItem)->setFieldWidth(fieldWidth);
      ((MenuPageTextEditItem*)pageItem)->setMaxLength(maxLength);
      ((MenuPageTextEditItem*)pageItem)->setScale(scale);
      ((MenuPageTextEditItem*)pageItem)->setColor(color);
      ((MenuPageTextEditItem*)pageItem)->setAlpha(alpha);

      // store page item without postfix names
      mPageItemNameMap[baseName] = pageItem;

      // if link between combobox table and button not yet made
      MenuPageEditableComboBoxItem::addTextEdit(baseName, (MenuPageTextEditItem*)pageItem);
      MenuPageEditableComboBoxItem::linkComboBoxToTextEdit(baseNameLineEdit, baseNameTable);
   }

   return pageItem;
}

void MenuPage::initializePageItems()
{
   // start reading settings
   mSettings->beginGroup(mTitle);

   std::string layerNameWithoutPostfix;
   std::string layerName;
   MenuPageItem* pageItem = 0;

   for (int l = 0; l < getLayerCount(); l++)
   {
      PSDLayer* layer = mRenderLayers[l];
      layerName = StringUtils::trim(getLayer(l)->getName());
      layerNameWithoutPostfix.clear();

      pageItem = 0;

      // combobox
      if (layerName.starts_with("combobox"))
      {
         pageItem = processComboBox(layer, layerName);
      }

      else if (layerName.starts_with("editablecombobox"))
      {
         pageItem = processEditableComboBox(layer, layerName);
      }

      // initialize checkboxes
      else if (layerName.starts_with("checkbox"))
      {
         pageItem = processCheckBox(layer, layerNameWithoutPostfix, layerName);
      }

      // initialize buttons
      else if (layerName.starts_with("button"))
      {
         pageItem = processButton(layer, layerName, layerNameWithoutPostfix);
      }

      else if (layerName.starts_with("label"))
      {
         // create a new page item
         pageItem = processLabel(layer, layerName);
      }

      // initialize lineedits
      else if (layerName.starts_with("lineedit"))
      {
         // create a new page item
         pageItem = processLineEdit(layer, layerName);
      }

      // initialize backgrounds
      else if (layerName.starts_with("background"))
      {
         // create a new page item
         pageItem = processBackground(layer, layerName);
      }

      // initialize pixmaps
      else if (layerName.starts_with("pixmap"))
      {
         // create a new page item
         pageItem = processPixmap(layer, layerName);
      }

      else if (layerName.starts_with("table_") && layerName.ends_with("_main"))
      {
         // create a new page item
         pageItem = processTableMain(layer, layerName);
      }

      else if (layerName.starts_with("table_") && (layerName.contains("_scroll_up") || layerName.contains("_scroll_down")))
      {
         // create a new page item
         pageItem = processTableScrollButtons(layer, layerName);
      }

      else if (layerName.starts_with("table_") && layerName.ends_with("_scrollbar"))
      {
         // create a new page item
         pageItem = processTableScrollBar(layer, layerName);
      }

      else if (layerName.starts_with("table_") && layerName.ends_with("_scroll_slider"))
      {
         // create a new page item
         pageItem = processTableScrollBarSlider(layer, layerName);
      }

      else if (layerName.starts_with("slider_") && !layerName.ends_with("_bar") && !layerName.ends_with("_icons"))
      {
         // create new slider
         pageItem = processSliderScrollBarIcons(layer, layerName);
      }

      else if (layerName.starts_with("image_scroll"))
      {
         pageItem = processScrollImage(layer, layerName);
      }

      else
      {
         // create a new page item
         pageItem = processDefaultItem(layer, layerName);
      }

      if (pageItem)
      {
         pageItem->initialize();
      }
   }

   // activate default item
   std::string defaultItem = mSettings->value("default").toString();
   if (mPageItemNameMap.contains(defaultItem))
   {
      mActiveItem = mPageItemNameMap[defaultItem];
      mActiveItem->activated();
      mActiveItem->setFocus(true);
   }

   mSettings->endGroup();
}

void MenuPage::initializeTabIndices()
{
   mSettings->beginGroup(mTitle);

   const auto indexItems = mSettings->value("tabindices").toStringList();

   if (!indexItems.empty())
   {
      int index = 0;
      for (const auto& key_str : indexItems)
      {
         if (mPageItemNameMap.contains(key_str))
         {
            mPageItemNameMap[key_str]->setTabIndex(index);
            index++;
         }
      }
   }

   mSettings->endGroup();
}

void MenuPage::tabPressed()
{
   int tabIndex = -1;

   // get current tab index
   if (mActiveItem)
   {
      tabIndex = mActiveItem->getTabIndex();
   }

   MenuPageItem* nextFocusItem = 0;

   // find greater tab index
   for (MenuPageItem* item : mPageItems)
   {
      if (item->getTabIndex() > tabIndex)
      {
         tabIndex = item->getTabIndex();
         nextFocusItem = item;
         break;
      }
   }

   // if no greater tab index found, continue with 0
   if (!nextFocusItem)
   {
      for (MenuPageItem* item : mPageItems)
      {
         if (item->getTabIndex() == 0)
         {
            tabIndex = item->getTabIndex();
            nextFocusItem = item;
            break;
         }
      }
   }

   if (nextFocusItem)
   {
      if (mActiveItem && mActiveItem != nextFocusItem)
      {
         mActiveItem->deactivated();
         mActiveItem = nextFocusItem;
         mActiveItem->activated();
      }
   }
}

MenuPageItem* MenuPage::getActiveItem() const
{
   return mActiveItem;
}

void MenuPage::setActiveItem(MenuPageItem* value)
{
   mActiveItem = value;
}

std::vector<MenuPageItem*> MenuPage::getItemsAt(int x, int y) const
{
   std::vector<MenuPageItem*> items;
   MenuPageItem* itemAtPos = 0;
   PSD::Layer* layer = 0;

   for (MenuPageItem* item : mPageItems)
   {
      if (item && item->isInteractive())
      {
         layer = item->getCurrentLayer();

         if (x > layer->getLeft() && x < layer->getLeft() + layer->getWidth() && y > layer->getTop() &&
             y < layer->getTop() + layer->getHeight())
         {
            itemAtPos = item;
            items.push_back(itemAtPos);
         }
      }
   }

   return items;
}

MenuPageItem* MenuPage::getFocussedItem() const
{
   MenuPageItem* focussedItem = 0;

   for (MenuPageItem* item : mPageItems)
   {
      if (item->isFocussed())
      {
         focussedItem = item;
         break;
      }
   }

   return focussedItem;
}

void MenuPage::mouseMoved(int x, int y)
{
   // check for layer collisions
   PSD::Layer* layer = 0;

   for (MenuPageItem* item : mPageItems)
   {
      if (item && item->isInteractive())
      {
         layer = item->getCurrentLayer();

         if (x > layer->getLeft() && x < layer->getLeft() + layer->getWidth() && y > layer->getTop() &&
             y < layer->getTop() + layer->getHeight())
         {
            if (!item->isFocussed())
            {
               if (item->isEnabled())
                  layerFocussedSignal(mFilename, layer->getName());

               item->setFocus(true);
            }

            if (item->hasNestedElements())
            {
               item->mouseMoved(x, y);
            }
         }
         else
         {
            if (item->isFocussed())
            {
               item->setFocus(false);
            }
         }

         // item is reading global mouse events
         if (item->isGrabbingMouseEvents())
         {
            if (item->isActive())
            {
               item->mouseMoved(x, y);
            }
         }
      }
   }
}

void MenuPage::mousePressed(int x, int y)
{
   PSD::Layer* layer = 0;
   bool focusSet = false;

   std::vector<MenuPageItem*> clickedItems;

   for (MenuPageItem* item : mPageItems)
   {
      if (item && item->isInteractive())
      {
         layer = item->getCurrentLayer();

         if (x > layer->getLeft() && x < layer->getLeft() + layer->getWidth() && y > layer->getTop() &&
             y < layer->getTop() + layer->getHeight())
         {
            clickedItems.push_back(item);
         }
      }
   }

   for (MenuPageItem* item : clickedItems)
   {
      if (item->isVisible() && item->isModal())
      {
         clickedItems.clear();
         clickedItems.push_back(item);
         break;
      }
   }

   for (MenuPageItem* item : clickedItems)
   {
      layer = item->getCurrentLayer();

      if (item->isActionRequestOnClickEnabled())
      {
         actionRequestSignal(mFilename, layer->getName());
      }

      item->activated();
      item->mousePressed(x, y);

      focusSet = true;

      if (mActiveItem != item)
      {
         if (mActiveItem)
            mActiveItem->deactivated();

         mActiveItem = item;
      }
   }

   if (!focusSet)
   {
      if (mActiveItem)
         mActiveItem->deactivated();

      mActiveItem = 0;
   }
}

void MenuPage::paste(const std::string& text)
{
   if (mActiveItem)
   {
      mActiveItem->paste(text);
   }
}

void MenuPage::mouseReleased()
{
   for (MenuPageItem* item : mPageItems)
   {
      if (item && item->isInteractive())
      {
         item->mouseReleased();
      }
   }
}

void MenuPage::setTitle(const std::string& title)
{
   mTitle = title;
}

void MenuPage::setFilename(const std::string& filename)
{
   mFilename = filename;
}

std::vector<MenuPageItem*>* MenuPage::getPageItems()
{
   return &mPageItems;
}

MenuPageItem* MenuPage::getPageItem(const std::string& layerName) const
{
   MenuPageItem* item = 0;

   auto it = mPageItemNameMap.find(layerName);

   if (it != mPageItemNameMap.end())
      item = it->second;

   return item;
}

void MenuPage::keyPressed(int key, const std::string& text)
{
   // check if any item has focus
   if (mActiveItem)
   {
      if (key == SDLK_TAB)
      {
         tabPressed();
      }
      else
      {
         mActiveItem->keyPressed(key, text);

         if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
         {
            actionRequestSignal(mFilename, mActiveItem->getCurrentLayer()->getName());
         }

         // in any case notify workflow a key was pressed
         actionKeyPressedSignal(mFilename, mActiveItem->getCurrentLayer()->getName(), key);
      }
   }
}

void MenuPage::setActive(bool active)
{
   mActive = active;
}

bool MenuPage::isActive()
{
   return mActive;
}

std::string MenuPage::getFilename() const
{
   return mFilename;
}

void MenuPage::setAnimation(MenuPageAnimation* animation)
{
   mAnimation = animation;
}

MenuPageAnimation* MenuPage::getAnimation()
{
   return mAnimation;
}

void MenuPage::deactivate()
{
   setActive(false);
}

void MenuPage::resetAnimation()
{
   mAnimation = 0;
}

void MenuPage::unFocusAllItems()
{
   for (MenuPageItem* item : mPageItems)
   {
      item->setFocus(false);
   }
}

void MenuPage::render()
{
   for (MenuPageItem* item : mPageItems)
   {
      item->draw();
   }
}

void MenuPage::actionRequestFromItem(const std::string& request)
{
   actionRequestSignal(mFilename, request);
}
