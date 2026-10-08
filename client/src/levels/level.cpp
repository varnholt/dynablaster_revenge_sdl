#include "level.h"
#include "framework/gldevice.h"
#include "materials/destructionmaterial.h"
#include "nodes/dummy.h"
#include "nodes/mesh.h"
#include "nodes/scenegraph.h"
#include "tools/datapaths.h"

#include <array>
#include <numbers>
#include <string>
#include <string_view>

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

   DataPaths::add(level_path.c_str());

   loadData();

   DataPaths::remove(level_path.c_str());
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
   activeDevice().clear();
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

void Level::addPlayerPosition(const PlayerInfo& player)
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

std::optional<std::reference_wrapper<Material>> Level::getStoryExtra(int32_t type) const
{
   const auto it = _story_extras.find(type);
   return it != _story_extras.end() ? it->second : std::nullopt;
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

Material& Level::getFlameExtra() const
{
   return _extra_flame.value();
}

Material& Level::getBombExtra() const
{
   return _extra_bomb.value();
}

Material& Level::getSpeedupExtra() const
{
   return _extra_speedup.value();
}

Material& Level::getKickExtra() const
{
   return _extra_kick.value();
}

Material& Level::getSkullExtra() const
{
   return _extra_skull.value();
}

SceneGraph& Level::getLevel() const
{
   return *_level;
}

SceneGraph& Level::getScene() const
{
   return *_scene;
}

SceneGraph& Level::getPlayers() const
{
   return *_players;
}

const std::vector<std::reference_wrapper<Node>>& Level::getDestructions() const
{
   return _destruct_anim;
}

Material& Level::getShadowBillboard() const
{
   return _shadow_billboards.value();
}

Material& Level::getShadowBlockBillboard() const
{
   return _shadow_blocks.value();
}

Material& Level::getBombMaterial() const
{
   return _bombs.value();
}

Material& Level::getSkullMaterial() const
{
   return _skulls.value();
}

Material& Level::getStoneMaterial() const
{
   return _stones.value();
}

Material& Level::getBlockMaterial() const
{
   return _blocks.value();
}

Material& Level::getDestructionMaterial() const
{
   return _destruction.value();
}

// loads the four block destruction animations, each as four copies rotated by 90 degrees
void Level::loadDestructions(Camera& shadow_camera)
{
   Material& destruction_material = _scene->addMaterial(
      std::make_unique<DestructionMaterial>("stone-unwrap", "diffuse_level", "specular_level", "shadow-cookie", shadow_camera)
   );
   _destruction = destruction_material;

   _destruction_templates = std::make_unique<SceneGraph>();

   constexpr std::array<std::string_view, 4> destructions = {
      "block-destruct0.hjb",
      "block-destruct1.hjb",
      "block-destruct2.hjb",
      "block-destruct.hjb",
   };

   for (const std::string_view destruction : destructions)
   {
      auto source = std::make_unique<SceneGraph>();
      if (!source->load(std::string(destruction)))
      {
         continue;
      }

      SceneGraph& scene = *_destruction_sources.emplace_back(std::move(source));

      // precalc tracks
      for (int32_t i = 0; i < scene.getChildCount(); i++)
      {
         scene.getChild(i).bakeAnimationTrack(160.0f);
      }

      // four copies, each rotated by 90 degrees
      Dummy& node = _destruction_templates->addNode(std::make_unique<Dummy>());
      for (int32_t rotation = 0; rotation < 4; rotation++)
      {
         const float angle = (rotation - 1) * std::numbers::pi_v<float> * 0.5f;
         const Matrix transform = Matrix::rotateZ(angle);

         Dummy& dummy = _destruction_templates->addNode(std::make_unique<Dummy>(), node);

         for (int32_t i = 0; i < scene.getChildCount(); i++)
         {
            const Node& child = scene.getChild(i);
            if (child.id() != Node::idMesh)
            {
               continue;
            }

            Mesh& mesh = _destruction_templates->addNode(std::make_unique<Mesh>(), dummy);
            mesh.copy(static_cast<const Mesh&>(child));
            mesh.transform(0.0f);
            mesh.createBoxMapping(true, Vector(-0.5f, -0.5f, 0.0f), Vector(0.5f, 0.5f, 1.0f), transform);
            mesh.setFrame(0.0f);
            destruction_material.addMesh(mesh);
         }
      }

      _destruct_anim.emplace_back(node);
   }
}
