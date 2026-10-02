// the PSD loader's layer groups: controls.psd keeps one player column in a group named "column".
#include "image/image.h"
#include "image/psd.h"

#include <SDL3/SDL.h>

#include <algorithm>
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

   const auto& layers = psd.getLayers();
   const auto dividers = std::ranges::count_if(layers, &PSD::Layer::isSectionDivider);
   const auto column_layers =
      std::ranges::count_if(layers, [](const auto& layer) { return layer.isImageLayer() && layer.getGroup() == "column"; });

   bool ok = true;
   ok &= check(dividers == 2, "one folder and one divider");
   ok &= check(column_layers == 20, "20 layers in the column group");

   const auto icon = psd.getLayer("p1-icon");
   const auto header = psd.getLayer("header");
   const auto column = psd.getLayer("column");
   ok &= check(icon != layers.end() && icon->getGroup() == "column", "p1-icon is in the column");
   ok &= check(header != layers.end() && header->getGroup().empty(), "header is top level");
   ok &= check(column != layers.end() && column->getSectionDivider() == PSD::Layer::SectionDivider::OpenFolder, "column is the folder");
   ok &= check(psd.getLayer("missing") == layers.end(), "unknown names are not found");

   // copies are plain values sharing the pixels
   PSD::Layer copy = *icon;
   copy.setName("p1-icon@2");
   copy.move(240, 0);
   ok &= check(copy.getLeft() == icon->getLeft() + 240 && copy.getTop() == icon->getTop(), "copy is shifted");
   ok &= check(copy.getImage().getData() == icon->getImage().getData(), "copy shares the pixels");
   ok &= check(copy.getGroup() == "column", "copy stays in its group");

   const size_t count = psd.getLayerCount();
   psd.addLayer(std::move(copy));
   ok &= check(psd.getLayerCount() == count + 1 && psd.getLayer("p1-icon@2") != psd.getLayers().end(), "copy added");

   SDL_Log(ok ? "psd groups test passed" : "psd groups test failed");
   return ok ? 0 : 1;
}
