#include "spherefragmentsdrawable.h"

// engine
#include "framework/framebuffer.h"
#include "gldevice.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "math/vector4.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "nodes/scenegraph.h"
#include "render/geometry.h"
#include "tools/filestream.h"
#include "tools/string.h"

// spherefragments
#include "blendquad.h"
#include "bombfuzegeometryvbo.h"
#include "bombsocketgeometryvbo.h"
#include "duplicatealpha.h"
#include "spherefragmentcontainer.h"
#include "spheregeometryvbo.h"

// postproduction
#include "postproduction/blurfilter.h"

#include <cstdint>

namespace
{
// creates the render target on first use, follows the viewport size afterwards
void updateFrameBuffer(std::unique_ptr<FrameBuffer>& frame_buffer, int32_t width, int32_t height)
{
   if (!frame_buffer)
   {
      frame_buffer = std::make_unique<FrameBuffer>(width, height);
   }
   else
   {
      frame_buffer->setResolution(width, height);
   }
}
}  // namespace

SphereFragmentsDrawable::SphereFragmentsDrawable(RenderDevice* device, bool visible) : Drawable(device, visible)
{
}

SphereFragmentsDrawable::~SphereFragmentsDrawable() = default;

void SphereFragmentsDrawable::initializeGL()
{
   FileStream::addPath("data/shaders");
   FileStream::addPath("data/effects/spherefragments/shaders");
   FileStream::addPath("data/effects/spherefragments/meshes");
   FileStream::addPath("data/effects/spherefragments/images");

   _alpha_duplicate = std::make_unique<DuplicateAlpha>();
   _blend_quad = std::make_unique<BlendQuad>();

   // earth
   _scene_graph_earth = std::make_unique<SceneGraph>();
   _scene_graph_earth->load("voronoisphere.hjb");
   auto* sphere = static_cast<Mesh*>(_scene_graph_earth->getNode("inner_sphere"));
   _bomb = std::make_unique<SphereGeometryVbo>(sphere->getPart(0));
   _bomb->initialize();

   // bomb socket and fuze
   _scene_graph_bomb = std::make_unique<SceneGraph>();
   _scene_graph_bomb->load("fuze_socket.hjb");

   auto* socket = static_cast<Mesh*>(_scene_graph_bomb->getNode("bomb_socket"));
   _socket = std::make_unique<BombSocketGeometryVbo>(socket->getPart(0));
   _socket->initialize();

   auto* fuze = static_cast<Mesh*>(_scene_graph_bomb->getNode("bomb_fuze"));
   _fuze = std::make_unique<BombFuzeGeometryVbo>(fuze->getPart(0));
   _fuze->initialize();

   removeFragments();

   _camera = Vector(0.0f, 0.0f, 2.6f);

   // default spherefragments_offset/scale style settings
   _position_offset = Vector(0.0f, 0.35f, 0.0f);
   _scale = 0.65f;

   _blur = std::make_unique<BlurFilter>();
   _blur->init();

   _fragment_container = std::make_unique<SphereFragmentContainer>(_scene_graph_earth.get());

   FileStream::removePath("data/effects/spherefragments/images");
}

void SphereFragmentsDrawable::projectionSetup()
{
   const float aspect = 9.0f / 16.0f;

   Matrix projection = Matrix::scale(_scale, _scale, _scale);
   projection = projection * Matrix::position(-_camera);
   projection = projection * Matrix::frustum(-1.0f, 1.0f, -aspect, aspect, 1.0f, 500.0f);

   static_cast<GLDevice*>(activeDevice)->setProjectionMatrix(projection);
}

void SphereFragmentsDrawable::paintGL()
{
   _fragment_container->animate(GlobalTime::Instance()->getTime());

   const auto width = static_cast<int32_t>(activeDevice->getWidth());
   const auto height = static_cast<int32_t>(activeDevice->getHeight());

   updateFrameBuffer(_earth_fb, width, height);
   updateFrameBuffer(_aura_fb, width, height);
   updateFrameBuffer(_bomb_fb, width, height);

   FrameBuffer::push();

   projectionSetup();

   // lighting pass 1

   // draw earth fragments once into a framebuffer, reuse later. clears to alpha=0: the alpha
   // channel drives the blend composite onto the menu below, so it must start fully transparent
   _earth_fb->bind();
   static_cast<GLDevice*>(mDevice)->clear(0.0f, 0.0f, 0.0f, 0.0f);

   // put bomb into zbuffer to black backside fragments
   _bomb->draw(Vector4(1, 1, 1, 0));
   _socket->draw(Vector4(1, 1, 1, 1));

   _fragment_container->drawFragments(_camera);
   _earth_fb->unbind();

   // atmosphere pass

   // create white mask from alpha channel
   _aura_fb->bind();
   static_cast<GLDevice*>(mDevice)->clear(0.0f, 0.0f, 0.0f, 0.0f);
   _alpha_duplicate->process(_earth_fb->texture(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));

   // blur white mask
   _blur->setRadius(30.0f * _aura_fb->width() / 1920.0f);
   _blur->process(_aura_fb->texture());
   _aura_fb->unbind();

   // lava glow pass
   _bomb_fb->bind();
   static_cast<GLDevice*>(mDevice)->clear(0.0f, 0.0f, 0.0f, 0.0f);
   _bomb->draw(Vector4(1, 1, 1, 1));

   // draw black fragments
   glEnable(GL_BLEND);
   _alpha_duplicate->process(_earth_fb->texture(), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
   glDisable(GL_BLEND);

   // blur
   _blur->setRadius(40.0f * _bomb_fb->width() / 1920.0f);
   _blur->process(_bomb_fb->texture());
   _bomb_fb->unbind();

   // lighting pass 2

   // draw bomb into fragment fb
   _earth_fb->bind();
   _bomb->draw(Vector4(1, 1, 1, 1));

   drawBombParts();

   _earth_fb->unbind();

   FrameBuffer::pop();

   // layer composition pass

   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);

   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

   const Vector4 color_atmosphere = Vector4(0.7f, 0.9f, 1.5f, 0.5f) * _alpha;
   const Vector4 color_fragments = Vector4(1.0f, 1.0f, 1.0f, 1.0f) * _alpha;
   const Vector4 color_glow = Vector4(4.0f, 4.0f, 4.0f, 1.0f) * _alpha;

   // atmosphere
   _blend_quad->process(_aura_fb->texture(), color_atmosphere, 1.0f, _position_offset);

   // fragments and inner sphere
   _blend_quad->process(_earth_fb->texture(), color_fragments, 1.0f, _position_offset);

   // add glow
   glBlendFunc(GL_ONE, GL_ONE);
   _blend_quad->process(_bomb_fb->texture(), color_glow, 1.0f, _position_offset);

   // cleanup
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SphereFragmentsDrawable::drawBombParts()
{
   _socket->draw(Vector4(1, 1, 1, 1));
   _fuze->draw(Vector4(1, 1, 1, 1));
}

void SphereFragmentsDrawable::removeFragments()
{
   for (int32_t i = 0; i < _scene_graph_bomb->getChildCount(); i++)
   {
      Node* node = _scene_graph_bomb->getChild(i);
      String name = node->name();
      const int32_t index = name.indexOf("_");
      if (index < 0)
      {
         continue;
      }

      name = name.mid(0, index);

      Node* fragment = _scene_graph_earth->getChild(name);
      if (fragment && fragment->id() == Node::idMesh)
      {
         static_cast<Mesh*>(fragment)->setVisible(false);
      }
   }
}
