#include "spherefragmentcontainer.h"

// spherefragments
#include "spherefragment.h"

// engine
#include "gldevice.h"
#include "image/image.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "render/texturepool.h"

namespace
{
constexpr size_t MESHES_PER_FRAGMENT = 16;
constexpr int32_t MAX_FRAGMENT_VERTICES = 200;
}  // namespace

SphereFragmentContainer::SphereFragmentContainer(Node* root)
{
   TexturePool* pool = TexturePool::Instance();

   Image normal_map("earth_normalmap");
   normal_map.buildNormalMap(256);  // strength of normal map

   _earth_texture = pool->getTexture("earth");
   _normal_map_texture = pool->getTexture(&normal_map);
   _lava_map_texture = pool->getTexture("bomb");

   _shader = activeDevice->loadShader("spherefragments-vert.glsl", "spherefragments-frag.glsl");

   _light_param = activeDevice->getParameterIndex("lightPosition");
   _camera_param = activeDevice->getParameterIndex("cameraPosition");
   _project_matrix_param = activeDevice->getParameterIndex("projection");

   _model_matrix_param = activeDevice->getParameterIndex("transformations");
   _fresnel_param = activeDevice->getParameterIndex("fresnelFactors");

   _texture_map_param = activeDevice->getParameterIndex("texturemap");
   _normal_map_param = activeDevice->getParameterIndex("normalmap");
   _specular_map_param = activeDevice->getParameterIndex("specularmap");
   _lava_map_param = activeDevice->getParameterIndex("lavamap");

   const Image order("order");

   std::vector<Mesh*> meshes;

   for (int32_t i = 0; i < root->getChildCount(); i++)
   {
      Node* node = root->getChild(i);
      if (!node->visible() || node->id() != Node::idMesh)
      {
         continue;
      }

      auto* mesh = static_cast<Mesh*>(node);
      if (mesh->getPart(0)->getVertexCount() >= MAX_FRAGMENT_VERTICES)
      {
         continue;
      }

      meshes.push_back(mesh);

      if (meshes.size() >= MESHES_PER_FRAGMENT)
      {
         _fragments.push_back(std::make_unique<SphereFragment>(meshes, &order));
         meshes.clear();
      }
   }

   if (!meshes.empty())
   {
      _fragments.push_back(std::make_unique<SphereFragment>(meshes, &order));
   }
}

SphereFragmentContainer::~SphereFragmentContainer() = default;

void SphereFragmentContainer::begin()
{
   activeDevice->setShader(_shader);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _earth_texture.getTexture());
   activeDevice->bindSampler(_texture_map_param, 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _normal_map_texture.getTexture());
   activeDevice->bindSampler(_normal_map_param, 1);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, _lava_map_texture.getTexture());
   activeDevice->bindSampler(_lava_map_param, 2);
}

void SphereFragmentContainer::animate(float time)
{
   Matrix rotation = Matrix::rotateY(time * 0.1745f);  // rotate the earth around its own y axis
   rotation = rotation * Matrix::rotateX(-1.0f);       // rotate the north pole towards the viewer
   rotation = rotation * Matrix::rotateY(1.0f);        // make earth rotate / instead of |

   time *= 0.5f;

   for (const auto& fragment : _fragments)
   {
      fragment->animate(time, rotation);
   }
}

void SphereFragmentContainer::drawFragments(const Vector& camera_position)
{
   begin();

   const Vector light_position(10, -10, -30);
   const Matrix projection = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix();

   activeDevice->setParameter(_light_param, light_position);
   activeDevice->setParameter(_camera_param, camera_position);
   activeDevice->setParameter(_project_matrix_param, projection);

   for (const auto& fragment : _fragments)
   {
      const int32_t count = fragment->getPartCount();

      activeDevice->setParameter(_model_matrix_param, fragment->getMatrices(), count);
      activeDevice->setParameter(_fresnel_param, fragment->getFresnelFactors(), count);

      fragment->draw();
   }

   end();
}

void SphereFragmentContainer::end()
{
   activeDevice->setShader(0);

   glActiveTexture(GL_TEXTURE0);
}
