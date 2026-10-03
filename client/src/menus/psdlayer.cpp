#include "psdlayer.h"
#include "framework/gldevice.h"
#include "image/image.h"
#include "math/matrix.h"
#include "render/texturepool.h"

#include <array>
#include <cstring>

PSDLayer::PSDLayer(PSD::Layer& layer, float z, bool unwrap) : _layer(layer), _opacity(layer.getOpacity() / 255.0)
{
   const int width = layer.getWidth();
   const int height = layer.getHeight();

   const int texture_width = width;
   const int texture_height = height;

   {
      Image image(texture_width, texture_height);
      const Image& source = layer.getImage();

      if (!unwrap)
      {
         image.scaled(source);
      }
      else
      {
         image.copy(0, 0, source, true);
      }

      _texture = TexturePool::Instance().getTexture(image, TexturePool::Trilinear | TexturePool::Clamp);
   }

   _u = static_cast<float>(width) / texture_width;
   _v = static_cast<float>(height) / texture_height;

   const std::array<Vertex, 4> vertices{
      Vertex(0, 0, z, 0, 0),
      Vertex(0, height, z, 0, _v),
      Vertex(width, 0, z, _u, 0),
      Vertex(width, height, z, _u, _v),
   };
   _vertex_buffer = activeDevice->createVertexBuffer(sizeof(vertices));
   std::memcpy(activeDevice->lockVertexBuffer(_vertex_buffer), vertices.data(), sizeof(vertices));
   activeDevice->unlockVertexBuffer(_vertex_buffer);

   constexpr std::array<uint16_t, 6> indices{0, 1, 2, 1, 3, 2};
   _index_buffer = activeDevice->createIndexBuffer(sizeof(indices));
   std::memcpy(activeDevice->lockIndexBuffer(_index_buffer), indices.data(), sizeof(indices));
   activeDevice->unlockIndexBuffer(_index_buffer);
}

PSD::Layer& PSDLayer::getLayer() const
{
   return _layer;
}

float PSDLayer::getU() const
{
   return _u;
}

float PSDLayer::getV() const
{
   return _v;
}

int PSDLayer::getWidth() const
{
   return _layer.get().getWidth();
}

int PSDLayer::getHeight() const
{
   return _layer.get().getHeight();
}

int PSDLayer::getLeft() const
{
   return _layer.get().getLeft();
}

int PSDLayer::getRight() const
{
   return getLeft() + getWidth();
}

int PSDLayer::getTop() const
{
   return _layer.get().getTop();
}

int PSDLayer::getBottom() const
{
   return _layer.get().getTop() + _layer.get().getHeight();
}

uint32_t PSDLayer::getTexture() const
{
   return _texture.getTexture();
}

uint32_t PSDLayer::getVertexBuffer() const
{
   return _vertex_buffer;
}

uint32_t PSDLayer::getIndexBuffer() const
{
   return _index_buffer;
}

float PSDLayer::getOpacity() const
{
   return _opacity;
}

void PSDLayer::setOpacity(float opacity)
{
   _opacity = opacity;
}

void PSDLayer::render(float x, float y, float alpha)
{
   Matrix world;
   world.translate(Vector(_layer.get().getLeft() + x, _layer.get().getTop() + y, 0.0f));
   activeDevice->push(world);

   glBindTexture(GL_TEXTURE_2D, _texture);

   activeDevice->setParameter(activeDevice->getParameterIndex("alpha"), _opacity * alpha);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(3 * sizeof(float)));

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice->pop();
}
