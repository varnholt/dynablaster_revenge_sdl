#include "storyfield.h"

#include "engine/gltf/gltfmodel.h"
#include "gles3.h"
#include "math/matrix.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace
{
constexpr float SCROLL_RATE = 4.0f;

std::unique_ptr<GltfModel> loadPiece(int32_t world, const char* piece)
{
   auto model = std::make_unique<GltfModel>(std::format("story/world{}/{}.glb", world, piece));

   // worlds without their own piece borrow it from the first one
   if (!model->isValid() && world != 1)
   {
      model = std::make_unique<GltfModel>(std::format("story/world1/{}.glb", piece));
   }

   return model;
}

Matrix at(float x, float y, float angle = 0.0f)
{
   return Matrix::rotateZ(angle) * Matrix::position(x + 0.5f, -(y + 0.5f), 0.0f);
}
}  // namespace

StoryField::StoryField() = default;

StoryField::~StoryField() = default;

void StoryField::setWorld(int32_t world)
{
   _world = std::clamp(world, 1, 8);
}

void StoryField::setSize(int32_t width, int32_t height)
{
   _width = width;
   _height = height;
   _snap = true;
}

void StoryField::follow(float player_x, float player_y, float dt)
{
   const float max_x = static_cast<float>(std::max(0, _width - SCREEN_WIDTH));
   const float max_y = static_cast<float>(std::max(0, _height - SCREEN_HEIGHT));

   const float target_x = std::clamp(player_x - SCREEN_WIDTH * 0.5f, 0.0f, max_x);
   const float target_y = std::clamp(player_y - SCREEN_HEIGHT * 0.5f, 0.0f, max_y);

   if (_snap)
   {
      _offset_x = target_x;
      _offset_y = target_y;
      _snap = false;
      return;
   }

   const float blend = 1.0f - std::exp(-SCROLL_RATE * dt);
   _offset_x += (target_x - _offset_x) * blend;
   _offset_y += (target_y - _offset_y) * blend;
}

float StoryField::getOffsetX() const
{
   return _offset_x;
}

float StoryField::getOffsetY() const
{
   return _offset_y;
}

StoryField::Kit& StoryField::getKit()
{
   auto it = _kits.find(_world);

   if (it == _kits.end())
   {
      Kit kit;
      kit._floor = loadPiece(_world, "floor");
      kit._border = loadPiece(_world, "border");
      kit._corner = loadPiece(_world, "corner");
      kit._surroundings = loadPiece(_world, "surroundings");
      it = _kits.emplace(_world, std::move(kit)).first;
   }

   return it->second;
}

void StoryField::render()
{
   Kit& kit = getKit();
   const GltfModel::Look look;
   constexpr float quarter = 0.5f * std::numbers::pi_v<float>;

   GltfModel::begin();

   // the ground never hides anything standing on it
   glDepthMask(GL_FALSE);

   // the land around the field, sized to it
   if (kit._surroundings->isValid())
   {
      const Matrix transform = Matrix::scale(static_cast<float>(_width), static_cast<float>(_height), 1.0f) *
                               Matrix::position(_width * 0.5f, -_height * 0.5f, 0.0f);
      kit._surroundings->draw(transform, -1, 0.0f, look);
   }

   if (kit._floor->isValid())
   {
      for (int32_t y = 0; y < _height; y++)
      {
         for (int32_t x = 0; x < _width; x++)
         {
            kit._floor->draw(at(static_cast<float>(x), static_cast<float>(y)), -1, 0.0f, look);
         }
      }
   }

   glDepthMask(GL_TRUE);

   if (kit._border->isValid())
   {
      // pieces face the field
      for (int32_t x = 0; x < _width; x++)
      {
         kit._border->draw(at(static_cast<float>(x), -1.0f), -1, 0.0f, look);
         kit._border->draw(at(static_cast<float>(x), static_cast<float>(_height), 2.0f * quarter), -1, 0.0f, look);
      }

      for (int32_t y = 0; y < _height; y++)
      {
         kit._border->draw(at(-1.0f, static_cast<float>(y), quarter), -1, 0.0f, look);
         kit._border->draw(at(static_cast<float>(_width), static_cast<float>(y), -quarter), -1, 0.0f, look);
      }

      GltfModel& corner = kit._corner->isValid() ? *kit._corner : *kit._border;
      corner.draw(at(-1.0f, -1.0f), -1, 0.0f, look);
      corner.draw(at(static_cast<float>(_width), -1.0f, -quarter), -1, 0.0f, look);
      corner.draw(at(-1.0f, static_cast<float>(_height), quarter), -1, 0.0f, look);
      corner.draw(at(static_cast<float>(_width), static_cast<float>(_height), 2.0f * quarter), -1, 0.0f, look);
   }

   GltfModel::end();
}
