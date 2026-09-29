#include "level.h"
#include "framework/gldevice.h"
#include "materials/destructionmaterial.h"
#include "nodes/dummy.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "tools/filestream.h"

#include <array>
#include <numbers>

namespace
{
constexpr auto LEVEL_CASTLE = "level-castle";
constexpr auto LEVEL_MANSION = "level-mansion";
constexpr auto LEVEL_SPACE = "level-space";
constexpr auto LEVEL_DUMMY = "level-dummy";
}  // namespace

Level::Level(LevelType level_type) : _level_type(level_type)
{
}

Level::~Level() = default;

void Level::abort()
{
   _aborted = true;
}

//! \return \c true if thread is aborted
bool Level::isAborted() const
{
   return _aborted;
}

void Level::load()
{
   const std::string level_path = "data/" + path();

   FileStream::addPath(level_path.c_str());

   loadData();

   FileStream::removePath(level_path.c_str());
}

void Level::loadData()
{
}

std::string Level::getLensFlareKey() const
{
   return {};
}

void Level::initialize()
{
}

void Level::draw()
{
}

void Level::drawBackground()
{
   activeDevice->clear();
}

void Level::animate(float /*dt*/)
{
}

void Level::reset()
{
}

std::string Level::path() const
{
   return getLevelDirectoryName(_level_type);
}

void Level::resetPlayerPositions()
{
   if (_camera_interpolation)
   {
      _camera_interpolation->resetPlayerPositions();
   }
}

void Level::startPositionUpdate(float width, float height, float dt)
{
   if (_camera_interpolation)
   {
      _camera_interpolation->startPositionUpdate(width, height, dt);
   }
}

void Level::addPlayerPosition(PlayerInfo* player)
{
   if (_camera_interpolation)
   {
      _camera_interpolation->addPlayerPosition(player);
   }
}

void Level::endPlayerPositionUpdate()
{
   if (_camera_interpolation)
   {
      _camera_interpolation->endPlayerPositionUpdate();
   }
}

bool Level::isPlayerMapEmpty() const
{
   return _camera_interpolation ? _camera_interpolation->isPlayerMapEmpty() : false;
}

Matrix Level::getCameraMatrix(float time, float scale)
{
   if (_camera_interpolation)
   {
      return _camera_interpolation->getCameraMatrix(time, scale);
   }

   return Matrix();
}

std::string Level::getLevelDirectoryName(Level::LevelType level_type)
{
   switch (level_type)
   {
      case LevelMansion:
         return LEVEL_MANSION;
      case LevelSpace:
         return LEVEL_SPACE;
      case LevelDummy:
         return LEVEL_DUMMY;
      default:
         return LEVEL_CASTLE;
   }
}

std::string Level::getLevelName(Level::LevelType level_type)
{
   switch (level_type)
   {
      case LevelMansion:
         return "the mansion";
      case LevelSpace:
         return "space";
      case LevelDummy:
         return "dummy";
      default:
         return "the castle";
   }
}

Material* Level::getFlameExtra() const
{
   return _extra_flame;
}

Material* Level::getBombExtra() const
{
   return _extra_bomb;
}

Material* Level::getSpeedupExtra() const
{
   return _extra_speedup;
}

Material* Level::getKickExtra() const
{
   return _extra_kick;
}

Material* Level::getSkullExtra() const
{
   return _extra_skull;
}

SceneGraph* Level::getLevel() const
{
   return _level.get();
}

SceneGraph* Level::getScene() const
{
   return _scene.get();
}

SceneGraph* Level::getPlayers() const
{
   return _players.get();
}

const Array<Node*>& Level::getDestructions() const
{
   return _destruct_anim;
}

Material* Level::getShadowBillboard() const
{
   return _shadow_billboards;
}

Material* Level::getShadowBlockBillboard() const
{
   return _shadow_blocks;
}

Material* Level::getBombMaterial() const
{
   return _bombs;
}

Material* Level::getSkullMaterial() const
{
   return _skulls;
}

Material* Level::getStoneMaterial() const
{
   return _stones;
}

Material* Level::getBlockMaterial() const
{
   return _blocks;
}

Material* Level::getDestructionMaterial() const
{
   return _destruction;
}

// loads the four block destruction animations, each as four copies rotated by 90 degrees
void Level::loadDestructions(Camera* shadow_camera)
{
   _destruction = new DestructionMaterial(_scene.get(), "stone-unwrap", "diffuse_level", "specular_level", "shadow-cookie", shadow_camera);

   constexpr std::array<const char*, 4> destructions = {
      "block-destruct0.hjb",
      "block-destruct1.hjb",
      "block-destruct2.hjb",
      "block-destruct.hjb",
   };

   for (const auto* destruction : destructions)
   {
      // never deleted, the mesh copies below keep pointers to its baked animation and motion mixer
      auto* scene = new SceneGraph();
      if (!scene->load(destruction))
      {
         delete scene;
         continue;
      }

      // precalc tracks
      for (int32_t i = 0; i < scene->getChildCount(); i++)
      {
         scene->getChild(i)->bakeAnimationTrack(160.0f);
      }

      // four copies, each rotated by 90 degrees
      auto* node = new Dummy(nullptr);
      for (int32_t rotation = 0; rotation < 4; rotation++)
      {
         const float angle = (rotation - 1) * std::numbers::pi_v<float> * 0.5f;
         const Matrix transform = Matrix::rotateZ(angle);

         auto* dummy = new Dummy(node);

         for (int32_t i = 0; i < scene->getChildCount(); i++)
         {
            Node* child = scene->getChild(i);
            if (child->id() != Node::idMesh)
            {
               continue;
            }

            auto* mesh = new Mesh(dummy);
            mesh->copy(*static_cast<Mesh*>(child));
            mesh->transform(0.0f);
            mesh->createBoxMapping(true, Vector(-0.5f, -0.5f, 0.0f), Vector(0.5f, 0.5f, 1.0f), transform);
            mesh->setFrame(0.0f);
            _destruction->addMesh(mesh);
         }
      }

      _destruct_anim.add(node);
   }
}
