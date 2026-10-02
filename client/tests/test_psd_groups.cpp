// the PSD loader's layer groups: controls.psd keeps one player column in a group named "column".
#include "image/image.h"
#include "image/psd.h"

#include <SDL3/SDL.h>

#include <memory>
#include <string>

namespace
{
bool check(bool condition, const char* what)
{
   if (!condition)
   {
      SDL_Log("FAILED: %s", what);
   }
   return condition;
}
}  // namespace

int main(int /*argc*/, char** /*argv*/)
{
   PSD psd;
   if (!check(psd.load("data/menus/controls.psd"), "load controls.psd"))
   {
      return 1;
   }

   int32_t column_layers = 0;
   int32_t markers = 0;
   for (int32_t i = 0; i < psd.getLayerCount(); i++)
   {
      const PSD::Layer* layer = psd.getLayer(i);
      if (layer->isGroupMarker())
      {
         markers++;
      }
      else if (layer->getGroup() == "column")
      {
         column_layers++;
      }
   }

   bool ok = true;
   ok &= check(markers == 2, "one folder and one divider");
   ok &= check(column_layers == 20, "20 layers in the column group");
   ok &= check(psd.getLayer("p1-icon") && psd.getLayer("p1-icon")->getGroup() == "column", "p1-icon is in the column");
   ok &= check(psd.getLayer("header") && psd.getLayer("header")->getGroup().empty(), "header is top level");
   ok &= check(psd.getLayer("column") && psd.getLayer("column")->getSection() != PSD::Layer::Section::None, "column is the folder");

   const PSD::Layer* icon = psd.getLayer("p1-icon");
   auto clone = icon->clone("p1-icon@2", 240, 0);
   ok &= check(std::string(clone->getName()) == "p1-icon@2", "clone is renamed");
   ok &= check(clone->getLeft() == icon->getLeft() + 240 && clone->getTop() == icon->getTop(), "clone is shifted");
   ok &= check(clone->getImage() && clone->getImage() != icon->getImage(), "clone owns its pixels");
   ok &= check(clone->getImage()->getWidth() == icon->getImage()->getWidth(), "clone has the same pixels");
   ok &= check(clone->getGroup() == "column", "clone stays in its group");

   const int32_t count = psd.getLayerCount();
   psd.addLayer(std::move(clone));
   ok &= check(psd.getLayerCount() == count + 1 && psd.getLayer("p1-icon@2"), "clone added");

   SDL_Log(ok ? "psd groups test passed" : "psd groups test failed");
   return ok ? 0 : 1;
}
