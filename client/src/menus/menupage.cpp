#include "menupage.h"
#include "psdlayer.h"

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

#include "stringutils.h"

#include <SDL3/SDL_keycode.h>

#include <algorithm>

namespace
{
void eraseAll(std::string& str, const std::string& sub)
{
   size_t position = 0;
   while ((position = str.find(sub, position)) != std::string::npos)
   {
      str.erase(position, sub.length());
   }
}

std::vector<std::string> splitByUnderscore(const std::string& s)
{
   std::vector<std::string> result;
   size_t start = 0;
   size_t position = s.find('_');

   while (position != std::string::npos)
   {
      result.push_back(s.substr(start, position - start));
      start = position + 1;
      position = s.find('_', start);
   }

   result.push_back(s.substr(start));

   return result;
}

bool containsPoint(const PSD::Layer* layer, int x, int y)
{
   return x > layer->getLeft() && x < layer->getLeft() + layer->getWidth() && y > layer->getTop() &&
          y < layer->getTop() + layer->getHeight();
}
}  // namespace

MenuPage::MenuPage() = default;

MenuPage::~MenuPage() = default;

template <typename T>
T* MenuPage::addPageItem()
{
   auto item = std::make_unique<T>();
   T* result = item.get();
   _page_items.push_back(std::move(item));
   return result;
}

void MenuPage::initialize()
{
   _settings = std::make_unique<Settings>("data/menus/menu.ini", Settings::IniFormat);

   initializeLayers();
   initializePageItems();
   initializeTabIndices();
}

void MenuPage::initializeLayers()
{
   load(_filename.c_str());
   repeatGroups();

   for (auto& layer : getLayers())
   {
      const std::string& layer_name = layer.getName();
      if (layer.isSectionDivider())
      {
         // group folders and dividers carry no pixels
         _render_layers.push_back(nullptr);
      }
      else if (layer_name.starts_with("background"))
      {
         _render_layers.push_back(std::make_unique<PSDLayer>(layer, -1.0f, false));
      }
      else
      {
         _render_layers.push_back(std::make_unique<PSDLayer>(layer, -1.0f));
      }
   }
}

MenuPageItem* MenuPage::processLabel(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPageLabelItem>();

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   // copies in a repeated group share their settings
   const std::string base_name = getInstanceBaseName(layer_name);
   const std::string font_name_key = base_name + "_font_name";
   const std::string font_x_offset_key = base_name + "_font_x_offset";
   const std::string font_y_offset_key = base_name + "_font_y_offset";
   const std::string max_chars_key = base_name + "_field_width";
   const std::string scale_key = base_name + "_scale";
   const std::string color_key = base_name + "_color";
   const std::string alpha_key = base_name + "_alpha";

   page_item->setLayerDrawn(_settings->value(base_name + "_draw_layer", true).toBool());
   if (_settings->value(base_name + "_centered", false).toBool())
   {
      page_item->setCenterWidth(static_cast<float>(layer->getWidth()));
   }

   page_item->setFontName(_settings->value(font_name_key, "default").toString());
   page_item->setFontXOffset(_settings->value(font_x_offset_key).toInt());
   page_item->setFontYOffset(_settings->value(font_y_offset_key).toInt());
   page_item->setMaxChars(_settings->value(max_chars_key).toInt());
   page_item->setScale(_settings->value(scale_key).toFloat());
   page_item->setColor(Color(_settings->value(color_key, "#FFFFFF").toString()));
   page_item->setAlpha(_settings->value(alpha_key, 255).toInt());

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   return page_item;
}

MenuPageItem* MenuPage::processLineEdit(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPageTextEditItem>();

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   // copies in a repeated group share their settings
   const std::string base_name = getInstanceBaseName(layer_name);
   const std::string font_name_key = base_name + "_font_name";
   const std::string font_x_offset_key = base_name + "_font_x_offset";
   const std::string font_y_offset_key = base_name + "_font_y_offset";
   const std::string field_width_key = base_name + "_field_width";
   const std::string field_max_length_key = base_name + "_max_length";
   const std::string scale_key = base_name + "_scale";
   const std::string color_key = base_name + "_color";
   const std::string alpha_key = base_name + "_alpha";

   page_item->setLayerDrawn(_settings->value(base_name + "_draw_layer", false).toBool());

   page_item->setFontName(_settings->value(font_name_key, "default").toString());
   page_item->setFontXOffset(_settings->value(font_x_offset_key).toInt());
   page_item->setFontYOffset(_settings->value(font_y_offset_key).toInt());
   page_item->setFieldWidth(_settings->value(field_width_key).toInt());
   page_item->setMaxLength(_settings->value(field_max_length_key).toInt());
   page_item->setScale(_settings->value(scale_key).toFloat());
   page_item->setColor(Color(_settings->value(color_key, "#FFFFFF").toString()));
   page_item->setAlpha(_settings->value(alpha_key, 255).toInt());

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   return page_item;
}

MenuPageItem* MenuPage::processBackground(PSDLayer* layer, std::string layer_name)
{
   bool added = false;
   MenuPageItem* page_item = nullptr;

   const std::string base_name = "background_" + splitByUnderscore(layer_name).at(1);

   if (!_page_item_name_map.contains(base_name))
   {
      added = true;
      page_item = addPageItem<MenuPageBackgroundItem>();
      _page_item_name_map[base_name] = page_item;
   }
   else
   {
      page_item = _page_item_name_map[base_name];
   }

   if (layer_name.ends_with("_active"))
   {
      page_item->setActiveLayer(layer);
      page_item->setInactiveLayer(layer);
   }
   else if (layer_name.contains("_gradient"))
   {
      MenuPageBackgroundItem::BackgroundColor color = MenuPageBackgroundItem::BackgroundColorBlue;

      if (layer_name.ends_with("_red"))
      {
         color = MenuPageBackgroundItem::BackgroundColorRed;
      }
      else if (layer_name.ends_with("_green"))
      {
         color = MenuPageBackgroundItem::BackgroundColorGreen;
      }

      dynamic_cast<MenuPageBackgroundItem*>(page_item)->addGradientLayer(layer, color);
   }

   return added ? page_item : nullptr;
}

MenuPageItem* MenuPage::processTableMain(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPageListItem>();

   std::string base_name = layer_name;
   eraseAll(base_name, "_main");

   // read lineedit properties
   const std::string font_name_key = base_name + "_font_name";
   const std::string font_x_offset_key = base_name + "_font_x_offset";
   const std::string font_y_offset_key = base_name + "_font_y_offset";
   const std::string field_width_key = base_name + "_field_width";
   const std::string scale_key = base_name + "_scale";
   const std::string row_height_key = base_name + "_row_height";

   page_item->setFontName(_settings->value(font_name_key, "default").toString());
   page_item->setFontXOffset(_settings->value(font_x_offset_key).toInt());
   page_item->setFontYOffset(_settings->value(font_y_offset_key).toInt());
   page_item->setFieldWidth(_settings->value(field_width_key).toInt());
   page_item->setScale(_settings->value(scale_key).toFloat());
   page_item->setRowHeight(_settings->value(row_height_key).toInt());

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   return page_item;
}

MenuPageItem* MenuPage::processTableScrollButtons(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPageItem>();

   page_item->setInteractive(true);

   const bool up = layer_name.contains("scroll_up");
   page_item->setAction(up ? "scroll_up" : "scroll_down");

   const std::string base_layer = "table_" + splitByUnderscore(layer_name).at(1) + "_main";

   // the scroll target lives on this page too, so the connections never outlive it
   auto* scroll_target = static_cast<MenuPageListItem*>(_page_item_name_map[base_layer]);

   if (up)
   {
      page_item->actionSignal.connect([scroll_target](const std::string&) { scroll_target->scrollUp(); });
   }
   else
   {
      page_item->actionSignal.connect([scroll_target](const std::string&) { scroll_target->scrollDown(); });
   }

   page_item->mouseReleasedSignal.connect([scroll_target]() { scroll_target->scrollStop(); });

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   return page_item;
}

MenuPageItem* MenuPage::processTableScrollBar(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPageItem>();

   page_item->setInteractive(true);

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   return page_item;
}

MenuPageItem* MenuPage::processTableScrollBarSlider(PSDLayer* layer, std::string layer_name)
{
   auto* scrollbar_item = addPageItem<MenuPageScrollbar>();

   scrollbar_item->setInteractive(true);

   const std::string table_name = splitByUnderscore(layer_name).at(1);
   const std::string scroll_area_layer = "table_" + table_name + "_scrollbar";

   // connect slider to table; both live on this page
   const std::string base_layer = "table_" + table_name + "_main";

   auto* table_item = static_cast<MenuPageListItem*>(_page_item_name_map[base_layer]);

   scrollbar_item->scrollToPercentageSignal.connect([table_item](float percent) { table_item->scrollToPercentage(percent); });

   MenuPageItem* scrollbar = _page_item_name_map[scroll_area_layer];
   scrollbar_item->setTop(scrollbar->getCurrentLayer()->getTop());
   scrollbar_item->setHeight(scrollbar->getCurrentLayer()->getHeight());

   // connect table back to slider
   table_item->scrollAnimationSignal.connect([scrollbar_item](float percent) { scrollbar_item->updateFromAnimation(percent); });

   // both layers are the same
   scrollbar_item->setActiveLayer(layer);
   scrollbar_item->setInactiveLayer(layer);

   // store page item without postfix names
   _page_item_name_map[layer_name] = scrollbar_item;

   return scrollbar_item;
}

MenuPageItem* MenuPage::processSliderScrollBarIcons(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPageSliderItem>();
   _page_item_name_map[layer_name] = page_item;

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   const std::string bar_name = layer_name + "_bar";
   if (const auto bar_layer = getLayer(bar_name); bar_layer != getLayers().end())
   {
      page_item->setMinimum(bar_layer->getLeft());
      page_item->setMaximum(bar_layer->getLeft() + bar_layer->getWidth());
   }

   return page_item;
}

MenuPageItem* MenuPage::processScrollImage(PSDLayer* layer, std::string layer_name)
{
   MenuPageScrollImageItem* scroll_image = nullptr;
   bool complete = false;

   // determine basename: image_scroll_cliprect or image_scroll
   std::string base_name = layer_name;
   const bool clip_rect = layer_name.contains("_cliprect");
   if (clip_rect)
   {
      eraseAll(base_name, "_cliprect");
   }

   if (_page_item_name_map.contains(base_name))
   {
      scroll_image = dynamic_cast<MenuPageScrollImageItem*>(_page_item_name_map[base_name]);

      // page item is now complete and can be initialized
      complete = true;
   }
   else
   {
      scroll_image = addPageItem<MenuPageScrollImageItem>();
      _page_item_name_map[base_name] = scroll_image;
   }

   if (clip_rect)
   {
      // we use the inactive layer as clipping layer
      scroll_image->setInactiveLayer(layer);
   }
   else
   {
      scroll_image->setActiveLayer(layer);
   }

   return complete ? scroll_image : nullptr;
}

MenuPageItem* MenuPage::processCheckBox(PSDLayer* layer, std::string layer_name_without_postfix, std::string layer_name)
{
   MenuPageItem* page_item = nullptr;
   layer_name_without_postfix = layer_name;
   eraseAll(layer_name_without_postfix, "_yes");
   eraseAll(layer_name_without_postfix, "_no");

   if (!_page_item_name_map.contains(layer_name_without_postfix))
   {
      page_item = addPageItem<MenuPageCheckBoxItem>();

      // store button action
      page_item->setAction(_settings->value(layer_name_without_postfix).toString());

      // store page item without postfix names
      _page_item_name_map[layer_name_without_postfix] = page_item;
   }
   else
   {
      // use previously assigned pageitem
      page_item = _page_item_name_map[layer_name_without_postfix];
   }

   if (layer_name.ends_with("_no"))
   {
      dynamic_cast<MenuPageCheckBoxItem*>(page_item)->setUncheckedLayer(layer);
   }
   else if (layer_name.ends_with("_yes"))
   {
      dynamic_cast<MenuPageCheckBoxItem*>(page_item)->setCheckedLayer(layer);
   }

   return page_item;
}

MenuPageItem* MenuPage::processPixmap(PSDLayer* layer, std::string layer_name)
{
   auto* page_item = addPageItem<MenuPagePixmapItem>();

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   return page_item;
}

MenuPageItem* MenuPage::processDefaultItem(PSDLayer* layer, std::string layer_name)
{
   if (layer_name.starts_with("unused_"))
   {
      return nullptr;
   }

   auto* page_item = addPageItem<MenuPageItem>();

   // both layers are the same
   page_item->setActiveLayer(layer);
   page_item->setInactiveLayer(layer);

   // store page item without postfix names
   _page_item_name_map[layer_name] = page_item;

   return page_item;
}

MenuPageItem* MenuPage::processButton(PSDLayer* layer, std::string layer_name, std::string layer_name_without_postfix)
{
   MenuPageItem* page_item = nullptr;
   layer_name_without_postfix = layer_name;
   eraseAll(layer_name_without_postfix, "_inactive");
   eraseAll(layer_name_without_postfix, "_active");

   if (!_page_item_name_map.contains(layer_name_without_postfix))
   {
      page_item = addPageItem<MenuPageButtonItem>();

      // store button action
      page_item->setAction(_settings->value(layer_name_without_postfix).toString());

      page_item->actionSignal.connect([this](const std::string& action) { actionRequestFromItem(action); });

      // store page item without postfix names
      _page_item_name_map[layer_name_without_postfix] = page_item;
   }
   else
   {
      // use previously assigned pageitem
      page_item = _page_item_name_map[layer_name_without_postfix];
   }

   if (layer_name.ends_with("_inactive"))
   {
      page_item->setInactiveLayer(layer);
   }
   else if (layer_name.ends_with("_active"))
   {
      page_item->setActiveLayer(layer);
   }

   return page_item;
}

MenuPageItem* MenuPage::processComboBox(PSDLayer* layer, std::string layer_name)
{
   MenuPageItem* page_item = nullptr;
   const std::vector<std::string> items = splitByUnderscore(layer_name);
   const std::string& item_name = items.at(1);

   // button, label or table
   const std::string& item_type = items.at(2);

   const std::string prefix = "combobox_" + item_name + "_";
   eraseAll(layer_name, prefix);
   const std::string postfix = layer_name;

   const std::string base_name = prefix + item_type;
   const std::string base_name_button = prefix + "button";
   const std::string base_name_table = prefix + "table";
   const std::string base_name_label = prefix + "label";

   const std::string item_type_lower = StringUtils::toLower(item_type);

   if (item_type_lower == "table")
   {
      MenuPageComboBoxItem* combo_box = nullptr;

      if (!_page_item_name_map.contains(base_name))
      {
         combo_box = addPageItem<MenuPageComboBoxItem>();

         MenuPageComboBoxItem::addComboBox(base_name, combo_box);
         MenuPageComboBoxItem::linkComboBoxToButton(base_name_button, base_name_table);

         // read lineedit properties
         const std::string font_name_key = base_name + "_font_name";
         const std::string font_x_offset_key = base_name + "_font_x_offset";
         const std::string font_y_offset_key = base_name + "_font_y_offset";
         const std::string scale_key = base_name + "_scale";
         const std::string row_height_key = base_name + "_row_height";

         combo_box->setFontName(_settings->value(font_name_key, "default").toString());
         combo_box->setFontXOffset(_settings->value(font_x_offset_key).toInt());
         combo_box->setFontYOffset(_settings->value(font_y_offset_key).toInt());
         combo_box->setScale(_settings->value(scale_key).toFloat());
         combo_box->setRowHeight(_settings->value(row_height_key).toInt());

         // store page item without postfix names
         _page_item_name_map[base_name] = combo_box;

         // if link between combobox table and button not yet made
         MenuPageComboBoxItem::addComboBox(base_name, combo_box);
         MenuPageComboBoxItem::linkComboBoxToButton(base_name_button, base_name_table);
      }
      else
      {
         combo_box = static_cast<MenuPageComboBoxItem*>(_page_item_name_map[base_name]);
      }

      page_item = combo_box;

      if (postfix == "table_selected_item")
      {
         combo_box->setLayerSelectedElement(layer);
      }
      else if (postfix == "table_focussed_item")
      {
         combo_box->setLayerFocussedElement(layer);
      }
      else if (postfix == "table_bg_first")
      {
         combo_box->setLayerFirstElement(layer);
      }
      else if (postfix == "table_bg_last")
      {
         combo_box->setLayerLastElement(layer);
      }
      else if (postfix == "table_bg_default")
      {
         combo_box->setLayerDefaultElement(layer);
      }
      else if (postfix == "table_gradient")
      {
         combo_box->setLayerGradientElement(layer);

         // maybe the gradient layer is good enough as active/inactive layer
         combo_box->setActiveLayer(layer);
         combo_box->setInactiveLayer(layer);
      }
   }
   else if (item_type_lower == "button")
   {
      // a standard button; deliberately not returned, so it is not initialized as page item
      MenuPageItem* button_item = nullptr;

      if (!_page_item_name_map.contains(base_name))
      {
         button_item = addPageItem<MenuPageButtonItem>();

         // store button action
         button_item->setAction(_settings->value(base_name).toString());

         button_item->actionSignal.connect([this](const std::string& action) { actionRequestFromItem(action); });

         // store page item without postfix names
         _page_item_name_map[base_name] = button_item;
      }
      else
      {
         // use previously assigned pageitem
         button_item = _page_item_name_map[base_name];
      }

      if (layer_name.ends_with("_inactive"))
      {
         button_item->setInactiveLayer(layer);
      }
      else if (layer_name.ends_with("_active"))
      {
         button_item->setActiveLayer(layer);
      }

      // if link between combobox table and button not yet made
      MenuPageComboBoxItem::addButton(base_name, static_cast<MenuPageButtonItem*>(button_item));
      MenuPageComboBoxItem::linkComboBoxToButton(base_name_button, base_name_table);
   }
   else if (item_type_lower == "label")
   {
      auto* label_item = addPageItem<MenuPageLabelItem>();
      page_item = label_item;

      // both layers are the same
      label_item->setActiveLayer(layer);
      label_item->setInactiveLayer(layer);

      const std::string font_name_key = base_name + "_font_name";
      const std::string font_x_offset_key = base_name + "_font_x_offset";
      const std::string font_y_offset_key = base_name + "_font_y_offset";
      const std::string scale_key = base_name + "_scale";

      label_item->setFontName(_settings->value(font_name_key, "default").toString());
      label_item->setFontXOffset(_settings->value(font_x_offset_key).toInt());
      label_item->setFontYOffset(_settings->value(font_y_offset_key).toInt());
      label_item->setScale(_settings->value(scale_key).toFloat());

      // store page item without postfix names
      _page_item_name_map[base_name] = label_item;

      // if link between combobox table and label not yet made
      MenuPageComboBoxItem::addLabel(base_name, label_item);
      MenuPageComboBoxItem::linkComboBoxToLabel(base_name_label, base_name_table);
   }

   return page_item;
}

MenuPageItem* MenuPage::processEditableComboBox(PSDLayer* layer, std::string layer_name)
{
   MenuPageItem* page_item = nullptr;
   const std::vector<std::string> items = splitByUnderscore(layer_name);
   const std::string& item_name = items.at(1);

   // button, lineedit or table
   const std::string& item_type = items.at(2);

   const std::string prefix = "editablecombobox_" + item_name + "_";
   eraseAll(layer_name, prefix);
   const std::string postfix = layer_name;

   const std::string base_name = prefix + item_type;
   const std::string base_name_button = prefix + "button";
   const std::string base_name_table = prefix + "table";
   const std::string base_name_line_edit = prefix + "lineedit";

   const std::string item_type_lower = StringUtils::toLower(item_type);

   if (item_type_lower == "table")
   {
      MenuPageEditableComboBoxItem* combo_box = nullptr;

      if (!_page_item_name_map.contains(base_name))
      {
         combo_box = addPageItem<MenuPageEditableComboBoxItem>();

         MenuPageComboBoxItem::addComboBox(base_name, combo_box);
         MenuPageComboBoxItem::linkComboBoxToButton(base_name_button, base_name_table);

         // read lineedit properties
         const std::string font_name_key = base_name + "_font_name";
         const std::string font_x_offset_key = base_name + "_font_x_offset";
         const std::string font_y_offset_key = base_name + "_font_y_offset";
         const std::string scale_key = base_name + "_scale";
         const std::string row_height_key = base_name + "_row_height";

         combo_box->setFontName(_settings->value(font_name_key, "default").toString());
         combo_box->setFontXOffset(_settings->value(font_x_offset_key).toInt());
         combo_box->setFontYOffset(_settings->value(font_y_offset_key).toInt());
         combo_box->setScale(_settings->value(scale_key).toFloat());
         combo_box->setRowHeight(_settings->value(row_height_key).toInt());

         // store page item without postfix names
         _page_item_name_map[base_name] = combo_box;

         // if link between combobox table and button not yet made
         MenuPageComboBoxItem::addComboBox(base_name, combo_box);
         MenuPageComboBoxItem::linkComboBoxToButton(base_name_button, base_name_table);
      }
      else
      {
         combo_box = static_cast<MenuPageEditableComboBoxItem*>(_page_item_name_map[base_name]);
      }

      page_item = combo_box;

      if (postfix == "table_selected_item")
      {
         combo_box->setLayerSelectedElement(layer);
      }
      else if (postfix == "table_focussed_item")
      {
         combo_box->setLayerFocussedElement(layer);
      }
      else if (postfix == "table_bg_first")
      {
         combo_box->setLayerFirstElement(layer);
      }
      else if (postfix == "table_bg_last")
      {
         combo_box->setLayerLastElement(layer);
      }
      else if (postfix == "table_bg_default")
      {
         combo_box->setLayerDefaultElement(layer);
      }
      else if (postfix == "table_gradient")
      {
         combo_box->setLayerGradientElement(layer);

         // maybe the gradient layer is good enough as active/inactive layer
         combo_box->setActiveLayer(layer);
         combo_box->setInactiveLayer(layer);
      }
   }
   else if (item_type_lower == "button")
   {
      // a standard button; deliberately not returned, so it is not initialized as page item
      MenuPageItem* button_item = nullptr;

      if (!_page_item_name_map.contains(base_name))
      {
         button_item = addPageItem<MenuPageButtonItem>();

         // store button action
         button_item->setAction(_settings->value(base_name).toString());

         button_item->actionSignal.connect([this](const std::string& action) { actionRequestFromItem(action); });

         // store page item without postfix names
         _page_item_name_map[base_name] = button_item;
      }
      else
      {
         // use previously assigned pageitem
         button_item = _page_item_name_map[base_name];
      }

      if (layer_name.ends_with("_inactive"))
      {
         button_item->setInactiveLayer(layer);
      }
      else if (layer_name.ends_with("_active"))
      {
         button_item->setActiveLayer(layer);
      }

      // if link between combobox table and button not yet made
      MenuPageComboBoxItem::addButton(base_name, static_cast<MenuPageButtonItem*>(button_item));
      MenuPageComboBoxItem::linkComboBoxToButton(base_name_button, base_name_table);
   }
   else if (item_type_lower == "lineedit")
   {
      auto* text_edit = addPageItem<MenuPageTextEditItem>();
      page_item = text_edit;

      // both layers are the same
      text_edit->setActiveLayer(layer);
      text_edit->setInactiveLayer(layer);

      const std::string font_name_key = base_name + "_font_name";
      const std::string font_x_offset_key = base_name + "_font_x_offset";
      const std::string font_y_offset_key = base_name + "_font_y_offset";
      const std::string field_width_key = base_name + "_field_width";
      const std::string field_max_length_key = base_name + "_max_length";
      const std::string scale_key = base_name + "_scale";
      const std::string color_key = base_name + "_color";
      const std::string alpha_key = base_name + "_alpha";

      text_edit->setFontName(_settings->value(font_name_key, "default").toString());
      text_edit->setFontXOffset(_settings->value(font_x_offset_key).toInt());
      text_edit->setFontYOffset(_settings->value(font_y_offset_key).toInt());
      text_edit->setFieldWidth(_settings->value(field_width_key).toInt());
      text_edit->setMaxLength(_settings->value(field_max_length_key).toInt());
      text_edit->setScale(_settings->value(scale_key).toFloat());
      text_edit->setColor(Color(_settings->value(color_key, "#FFFFFF").toString()));
      text_edit->setAlpha(_settings->value(alpha_key, 255).toInt());

      // store page item without postfix names
      _page_item_name_map[base_name] = text_edit;

      // if link between combobox table and textedit not yet made
      MenuPageEditableComboBoxItem::addTextEdit(base_name, text_edit);
      MenuPageEditableComboBoxItem::linkComboBoxToTextEdit(base_name_line_edit, base_name_table);
   }

   return page_item;
}

/*!
   a layer group can be repeated, e.g. one column per player:

   repeat_groups = column
   column_repeat_count = 4          // instances
   column_repeat_spacing = 240      // horizontal distance between them in the PSD layout
   column_repeat_layers = a,b       // layers outside the group that belong to every instance

   each instance's layers are named <layer>@<n>, n = 1..count; instance 1 is the PSD's own.
*/
void MenuPage::repeatGroups()
{
   _settings->beginGroup(_title);
   const auto groups = _settings->value("repeat_groups").toStringList();

   for (const auto& group : groups)
   {
      const int32_t count = _settings->value(group + "_repeat_count", 1).toInt();
      const int32_t spacing = _settings->value(group + "_repeat_spacing", 0).toInt();
      const auto extra_layers = _settings->value(group + "_repeat_layers").toStringList();

      // indices, appending the copies reallocates the layers
      std::vector<size_t> members;
      for (size_t l = 0; l < getLayerCount(); l++)
      {
         const PSD::Layer& layer = getLayer(l);
         if (layer.isImageLayer() && (layer.getGroup() == group || std::ranges::contains(extra_layers, StringUtils::trim(layer.getName()))))
         {
            members.push_back(l);
         }
      }

      auto& instances = _group_instances[group];
      instances.assign(static_cast<size_t>(std::max(count, 1)), {});
      _group_instance_offsets[group].assign(instances.size(), 0);

      for (const size_t member : members)
      {
         const std::string name = StringUtils::trim(getLayer(member).getName());
         for (int32_t index = 2; index <= count; index++)
         {
            // copies share the pixels
            PSD::Layer copy = getLayer(member);
            copy.setName(getInstanceName(name, index));
            copy.move(spacing * (index - 1), 0);
            instances[index - 1].push_back(getLayerCount());
            addLayer(std::move(copy));
         }
         getLayer(member).setName(getInstanceName(name, 1));
         instances.front().push_back(member);
      }
   }

   _settings->endGroup();
}

int32_t MenuPage::getGroupInstanceCount(const std::string& group) const
{
   const auto it = _group_instances.find(group);
   return it != _group_instances.end() ? static_cast<int32_t>(it->second.size()) : 0;
}

void MenuPage::setGroupInstanceOffset(const std::string& group, int32_t index, int32_t offset)
{
   const auto instances = _group_instances.find(group);
   if (instances == _group_instances.end() || index < 1 || index > static_cast<int32_t>(instances->second.size()))
   {
      return;
   }

   int32_t& current = _group_instance_offsets[group][index - 1];
   for (const size_t layer : instances->second[index - 1])
   {
      getLayer(layer).move(offset - current, 0);
   }
   current = offset;
}

std::string MenuPage::getInstanceName(const std::string& layer_name, int32_t index)
{
   return layer_name + "@" + std::to_string(index);
}

std::string MenuPage::getInstanceBaseName(const std::string& layer_name)
{
   const auto at = layer_name.rfind('@');
   if (at == std::string::npos || at + 1 == layer_name.size() ||
       !std::ranges::all_of(layer_name.substr(at + 1), [](char c) { return c >= '0' && c <= '9'; }))
   {
      return layer_name;
   }
   return layer_name.substr(0, at);
}

void MenuPage::initializePageItems()
{
   _settings->beginGroup(_title);

   // plain layers start hidden if they're hidden in the PSD (opt-in, older pages toggle theirs in code)
   const bool respect_layer_visibility = _settings->value("respect_layer_visibility", false).toBool();

   std::string layer_name_without_postfix;

   for (size_t l = 0; l < getLayerCount(); l++)
   {
      PSDLayer* layer = _render_layers[l].get();
      if (!layer)
      {
         continue;
      }

      const std::string layer_name = StringUtils::trim(getLayer(l).getName());
      layer_name_without_postfix.clear();

      MenuPageItem* page_item = nullptr;

      // menu.ini can give plain layers a type: <layer>_item = label | lineedit | clickable
      const std::string item_type = _settings->value(getInstanceBaseName(layer_name) + "_item").toString();

      if (item_type == "label")
      {
         page_item = processLabel(layer, layer_name);
      }
      else if (item_type == "lineedit")
      {
         page_item = processLineEdit(layer, layer_name);
      }
      else if (item_type == "clickable")
      {
         page_item = processDefaultItem(layer, layer_name);
         if (page_item)
         {
            page_item->setInteractive(true);
            if (respect_layer_visibility)
            {
               page_item->setVisible(getLayer(l).isVisible());
            }
         }
      }
      else if (layer_name.starts_with("combobox"))
      {
         page_item = processComboBox(layer, layer_name);
      }
      else if (layer_name.starts_with("editablecombobox"))
      {
         page_item = processEditableComboBox(layer, layer_name);
      }
      else if (layer_name.starts_with("checkbox"))
      {
         page_item = processCheckBox(layer, layer_name_without_postfix, layer_name);
      }
      else if (layer_name.starts_with("button"))
      {
         page_item = processButton(layer, layer_name, layer_name_without_postfix);
      }
      else if (layer_name.starts_with("label"))
      {
         page_item = processLabel(layer, layer_name);
      }
      else if (layer_name.starts_with("lineedit"))
      {
         page_item = processLineEdit(layer, layer_name);
      }
      else if (layer_name.starts_with("background"))
      {
         page_item = processBackground(layer, layer_name);
      }
      else if (layer_name.starts_with("pixmap"))
      {
         page_item = processPixmap(layer, layer_name);
      }
      else if (layer_name.starts_with("table_") && layer_name.ends_with("_main"))
      {
         page_item = processTableMain(layer, layer_name);
      }
      else if (layer_name.starts_with("table_") && (layer_name.contains("_scroll_up") || layer_name.contains("_scroll_down")))
      {
         page_item = processTableScrollButtons(layer, layer_name);
      }
      else if (layer_name.starts_with("table_") && layer_name.ends_with("_scrollbar"))
      {
         page_item = processTableScrollBar(layer, layer_name);
      }
      else if (layer_name.starts_with("table_") && layer_name.ends_with("_scroll_slider"))
      {
         page_item = processTableScrollBarSlider(layer, layer_name);
      }
      else if (layer_name.starts_with("slider_") && !layer_name.ends_with("_bar") && !layer_name.ends_with("_icons"))
      {
         page_item = processSliderScrollBarIcons(layer, layer_name);
      }
      else if (layer_name.starts_with("image_scroll"))
      {
         page_item = processScrollImage(layer, layer_name);
      }
      else
      {
         page_item = processDefaultItem(layer, layer_name);
         if (page_item && respect_layer_visibility)
         {
            page_item->setVisible(getLayer(l).isVisible());
         }
      }

      if (page_item)
      {
         page_item->initialize();
      }
   }

   // activate default item
   const std::string default_item = _settings->value("default").toString();
   if (_page_item_name_map.contains(default_item))
   {
      _active_item = _page_item_name_map[default_item];
      _active_item->activated();
      _active_item->setFocus(true);
   }

   _settings->endGroup();
}

void MenuPage::initializeTabIndices()
{
   _settings->beginGroup(_title);

   const auto index_items = _settings->value("tabindices").toStringList();

   int index = 0;
   for (const auto& key : index_items)
   {
      if (_page_item_name_map.contains(key))
      {
         _page_item_name_map[key]->setTabIndex(index);
         index++;
      }
   }

   _settings->endGroup();
}

void MenuPage::tabPressed()
{
   // get current tab index
   const int tab_index = _active_item ? _active_item->getTabIndex() : -1;

   // find greater tab index
   auto next = std::ranges::find_if(_page_items, [tab_index](const auto& item) { return item->getTabIndex() > tab_index; });

   // if no greater tab index found, continue with 0
   if (next == _page_items.end())
   {
      next = std::ranges::find_if(_page_items, [](const auto& item) { return item->getTabIndex() == 0; });
   }

   if (next != _page_items.end())
   {
      MenuPageItem* next_focus_item = next->get();

      if (_active_item && _active_item != next_focus_item)
      {
         _active_item->deactivated();
         _active_item = next_focus_item;
         _active_item->activated();
      }
   }
}

MenuPageItem* MenuPage::getActiveItem() const
{
   return _active_item;
}

void MenuPage::setActiveItem(MenuPageItem* value)
{
   _active_item = value;
}

std::vector<MenuPageItem*> MenuPage::getItemsAt(int x, int y) const
{
   std::vector<MenuPageItem*> items;

   for (const auto& item : _page_items)
   {
      if (item && item->isInteractive() && containsPoint(item->getCurrentLayer(), x, y))
      {
         items.push_back(item.get());
      }
   }

   return items;
}

MenuPageItem* MenuPage::getFocussedItem() const
{
   const auto iterator = std::ranges::find_if(_page_items, [](const auto& item) { return item->isFocussed(); });
   return iterator != _page_items.end() ? iterator->get() : nullptr;
}

void MenuPage::mouseMoved(int x, int y)
{
   for (const auto& item : _page_items)
   {
      if (!item || !item->isInteractive())
      {
         continue;
      }

      PSD::Layer* layer = item->getCurrentLayer();

      if (containsPoint(layer, x, y))
      {
         if (!item->isFocussed())
         {
            if (item->isEnabled())
            {
               layerFocussedSignal(_filename, layer->getName());
            }

            item->setFocus(true);
         }

         if (item->hasNestedElements())
         {
            item->mouseMoved(x, y);
         }
      }
      else if (item->isFocussed())
      {
         item->setFocus(false);
      }

      // item is reading global mouse events
      if (item->isGrabbingMouseEvents() && item->isActive())
      {
         item->mouseMoved(x, y);
      }
   }
}

void MenuPage::mousePressed(int x, int y)
{
   bool focus_set = false;

   std::vector<MenuPageItem*> clicked_items;

   for (const auto& item : _page_items)
   {
      if (item && item->isInteractive() && containsPoint(item->getCurrentLayer(), x, y))
      {
         clicked_items.push_back(item.get());
      }
   }

   // a visible modal item swallows the click
   const auto modal = std::ranges::find_if(clicked_items, [](MenuPageItem* item) { return item->isVisible() && item->isModal(); });
   if (modal != clicked_items.end())
   {
      clicked_items = {*modal};
   }

   for (MenuPageItem* item : clicked_items)
   {
      const PSD::Layer* layer = item->getCurrentLayer();

      if (item->isActionRequestOnClickEnabled())
      {
         actionRequestSignal(_filename, layer->getName());
      }

      item->activated();
      item->mousePressed(x, y);

      focus_set = true;

      if (_active_item != item)
      {
         if (_active_item)
         {
            _active_item->deactivated();
         }

         _active_item = item;
      }
   }

   if (!focus_set)
   {
      if (_active_item)
      {
         _active_item->deactivated();
      }

      _active_item = nullptr;
   }
}

void MenuPage::paste(const std::string& text)
{
   if (_active_item)
   {
      _active_item->paste(text);
   }
}

void MenuPage::mouseReleased()
{
   for (const auto& item : _page_items)
   {
      if (item && item->isInteractive())
      {
         item->mouseReleased();
      }
   }
}

void MenuPage::setTitle(const std::string& title)
{
   _title = title;
}

void MenuPage::setFilename(const std::string& filename)
{
   _filename = filename;
}

const std::vector<std::unique_ptr<MenuPageItem>>& MenuPage::getPageItems() const
{
   return _page_items;
}

MenuPageItem* MenuPage::getPageItem(const std::string& layer_name) const
{
   const auto iterator = _page_item_name_map.find(layer_name);
   return iterator != _page_item_name_map.end() ? iterator->second : nullptr;
}

void MenuPage::keyPressed(int key, const std::string& text)
{
   // check if any item has focus
   if (!_active_item)
   {
      return;
   }

   if (key == SDLK_TAB)
   {
      tabPressed();
   }
   else
   {
      _active_item->keyPressed(key, text);

      if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
      {
         actionRequestSignal(_filename, _active_item->getCurrentLayer()->getName());
      }

      // in any case notify workflow a key was pressed
      actionKeyPressedSignal(_filename, _active_item->getCurrentLayer()->getName(), key);
   }
}

void MenuPage::setActive(bool active)
{
   _active = active;
}

bool MenuPage::isActive()
{
   return _active;
}

std::string MenuPage::getFilename() const
{
   return _filename;
}

void MenuPage::setAnimation(MenuPageAnimation* animation)
{
   _animation = animation;
}

MenuPageAnimation* MenuPage::getAnimation()
{
   return _animation;
}

void MenuPage::deactivate()
{
   setActive(false);
}

void MenuPage::resetAnimation()
{
   _animation = nullptr;
}

void MenuPage::unFocusAllItems()
{
   for (const auto& item : _page_items)
   {
      item->setFocus(false);
   }
}

void MenuPage::render()
{
   for (const auto& item : _page_items)
   {
      item->draw();
   }
}

void MenuPage::actionRequestFromItem(const std::string& action_request)
{
   actionRequestSignal(_filename, action_request);
}
